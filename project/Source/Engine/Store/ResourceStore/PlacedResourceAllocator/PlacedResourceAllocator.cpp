#include "PlacedResourceAllocator.hpp"

/// @brief 初期化
/// @param device 
/// @param pageSizeInBytes 
/// @param heapFlags 
void Detail::PlacedResourceAllocator::Initialize(ID3D12Device* device, uint64_t pageSizeInBytes)
{
    device_ = device;
    pageSize_ = (pageSizeInBytes + 65535) & ~65535; // 64KBアライメント

    // 最初のページを作成
    AddNewPage();
}

/// @brief メモリの割り当て（デフォルトはD3D12の標準である64KBアライメント）
/// @param size 
/// @param alignment 
/// @return 
Detail::PlacedAllocation Detail::PlacedResourceAllocator::Allocate(uint64_t size, uint64_t alignment)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // 既存のページから空きを探す
    for (uint32_t pageIdx = 0; pageIdx < pages_.size(); ++pageIdx)
    {
        auto& page = pages_[pageIdx];
        for (auto it = page.freeBlocks.begin(); it != page.freeBlocks.end(); ++it)
        {
            // アライメントを考慮してオフセットを計算
            uint64_t alignedOffset = (it->offset + alignment - 1) & ~(alignment - 1);
            uint64_t padding = alignedOffset - it->offset;

            // 割り当て可能かどうかを確認
            if (it->size >= size + padding)
            {
                // 割り当て
                PlacedAllocation alloc;
                alloc.heapIndex = pageIdx;
                alloc.offset = alignedOffset;
                alloc.size = size;

                // 残りの空きブロックのサイズを計算
                uint64_t remaining = it->size - (size + padding);
                if (padding > 0) page.freeBlocks.insert(it, { it->offset, padding });

                // 残りの空きブロックを更新
                if (remaining > 0)
                {
                    it->offset = alignedOffset + size;
                    it->size = remaining;
                }
                else
                {
                    // 空きブロックがなくなった場合はリストから削除
                    page.freeBlocks.erase(it);
                }

                return alloc;
            }
        }
    }

    // 既存ページに空きがない場合、要求サイズが1ページ分より大きければエラーとするか、専用の巨大ページを作る
    if (size > pageSize_) throw std::runtime_error("要求サイズがページサイズを超えています");

    // 新しいページを追加して再帰（またはループ）処理
    AddNewPage();

    // 追加した最新のページから割り当て
    uint32_t newPageIdx = static_cast<uint32_t>(pages_.size() - 1);
    auto& page = pages_[newPageIdx];
    auto it = page.freeBlocks.begin(); // 確実に入る

    // 新しいページの最初の空きブロックから割り当て
    uint64_t alignedOffset = (it->offset + alignment - 1) & ~(alignment - 1);
    uint64_t padding = alignedOffset - it->offset;

    // 割り当て
    PlacedAllocation alloc;
    alloc.heapIndex = newPageIdx;
    alloc.offset = alignedOffset;
    alloc.size = size;

    // 残りの空きブロックのサイズを計算
    uint64_t remaining = it->size - (size + padding);

    // 空きブロックの更新
    if (padding > 0)page.freeBlocks.insert(it, { it->offset, padding });

    // 残りの空きブロックを更新
    if (remaining > 0)
    {
        it->offset = alignedOffset + size;
        it->size = remaining;
    }
    else
    {
        // 空きブロックがなくなった場合はリストから削除
        page.freeBlocks.erase(it);
    }

    return alloc;
}

/// @brief メモリの解放
/// @param allocation 
void Detail::PlacedResourceAllocator::Free(const PlacedAllocation& allocation)
{
    if (!allocation.IsValid()) return;
    std::lock_guard<std::mutex> lock(mutex_);

    auto& page = pages_[allocation.heapIndex];

    // 解放されたブロックをフリーブロックリストに追加し、隣接するブロックとマージする
    auto it = page.freeBlocks.begin();
    while (it != page.freeBlocks.end() && it->offset < allocation.offset) ++it;
    it = page.freeBlocks.insert(it, { allocation.offset, allocation.size });

    // 前のブロックとマージ
    if (it != page.freeBlocks.begin())
    {
        // 前のブロックが隣接している場合はマージ
        auto prev = std::prev(it);
        if (prev->offset + prev->size == it->offset)
        {
            prev->size += it->size;
            page.freeBlocks.erase(it);
            it = prev;
        }
    }

    // 次のブロックとマージ
    auto next = std::next(it);
    if (next != page.freeBlocks.end())
    {
        // 隣接している場合はマージ
        if (it->offset + it->size == next->offset)
        {
            it->size += next->size;
            page.freeBlocks.erase(next);
        }
    }
}

/// @brief 新しいヒープページを追加する
void Detail::PlacedResourceAllocator::AddNewPage()
{
    // ヒープ設定
    D3D12_HEAP_DESC heapDesc = {};
    heapDesc.SizeInBytes = pageSize_;
    heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    heapDesc.Flags = heapFlags_;

    // 新しいヒープを作成
    Microsoft::WRL::ComPtr<ID3D12Heap> newHeap;
    if (FAILED(device_->CreateHeap(&heapDesc, IID_PPV_ARGS(&newHeap))))
    {
        throw std::runtime_error("新しい ID3D12Heap の生成に失敗しました");
    }

    // 新しいページを追加
    HeapPage page;
    page.heap = newHeap;
    page.freeBlocks.push_back({ 0, pageSize_ });
    pages_.push_back(std::move(page));
}
