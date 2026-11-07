#include "SubAllocator.hpp"

/// @brief コンストラクタ
/// @param capacity 
Detail::SubAllocator::SubAllocator(uint64_t capacity) : capacity_(capacity)
{
    // 初期状態ではバッファ全体が1つの空きブロック
    FreeBlock initialBlock;
    initialBlock.offset = 0;
    initialBlock.size = capacity;
    freeBlocks_.push_back(initialBlock);
}

/// @brief メモリを確保する
/// @param size
/// @param alignment 
/// @return
Detail::SubAllocation Detail::SubAllocator::Allocate(uint64_t size, uint64_t alignment)
{
    for (auto it = freeBlocks_.begin(); it != freeBlocks_.end(); ++it)
    {
        // アライメントを考慮して、ブロックの開始位置を調整
        uint64_t remainder = it->offset % alignment;
        uint64_t alignedOffset = it->offset;
        if (remainder != 0)
        {
            alignedOffset += (alignment - remainder);
        }

        // アライメント調整によって生じる前方パディングのサイズ
        uint64_t padding = alignedOffset - it->offset;

        // このブロックに十分な空き容量があるか
        if (it->size >= size + padding)
        {
            SubAllocation allocation;
            allocation.offset = alignedOffset;
            allocation.size = size;

            // アライメント調整によって生じた前方パディング分を新しい空きブロックとして分離
            if (padding > 0)
            {
                FreeBlock beforeBlock;
                beforeBlock.offset = it->offset;
                beforeBlock.size = padding;
                freeBlocks_.insert(it, beforeBlock);
            }

            // 割り当て後の残りサイズ
            uint64_t remainingSize = it->size - (size + padding);
            if (remainingSize > 0)
            {
                // 後方に空きが残る場合、現在のブロック情報を更新
                it->offset = alignedOffset + size;
                it->size = remainingSize;
            }
            else
            {
                // 完全に使い切った場合、ブロックをリストから削除
                freeBlocks_.erase(it);
            }

            return allocation;
        }
    }

    // 空きがない場合
    return SubAllocation{ 0, 0 };
}

/// @brief メモリを解放し、必要に応じて隣接する空きブロックと結合（マージ）する
/// @param allocation
void Detail::SubAllocator::Free(const SubAllocation& allocation)
{
    if (!allocation.IsValid()) return;

    FreeBlock newBlock;
    newBlock.offset = allocation.offset;
    newBlock.size = allocation.size;

    // オフセット順になるように挿入位置を探索
    auto it = std::lower_bound(freeBlocks_.begin(), freeBlocks_.end(), newBlock,
        [](const FreeBlock& a, const FreeBlock& b) {
            return a.offset < b.offset;
        });

    // リストに挿入
    it = freeBlocks_.insert(it, newBlock);

    // 前後の空きブロックとマージできるか確認
    MergeBlocks(it);
}

/// @brief 指定したイテレータのブロックを前後のブロックと結合する
/// @param it 
void Detail::SubAllocator::MergeBlocks(std::list<FreeBlock>::iterator it)
{
    // 次のブロックとマージ可能かチェック
    auto nextIt = std::next(it);
    if (nextIt != freeBlocks_.end() && it->offset + it->size == nextIt->offset)
    {
        it->size += nextIt->size;
        freeBlocks_.erase(nextIt);
    }

    // 前のブロックとマージ可能かチェック
    if (it != freeBlocks_.begin())
    {
        auto prevIt = std::prev(it);
        if (prevIt->offset + prevIt->size == it->offset)
        {
            prevIt->size += it->size;
            freeBlocks_.erase(it);
        }
    }
}
