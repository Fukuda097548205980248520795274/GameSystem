#include "IndexBufferResource.hpp"
#include "Func/ResourceFunc/ResourceFunc.hpp"
#include <cassert>

/// @brief デストラクタ
Detail::IndexBufferResource::~IndexBufferResource()
{
    if (allocator_)
    {
        for (auto& region : regions_)
        {
            if (region.IsValid()) allocator_->Free(region);
        }
    }
}

/// @brief 初期化
/// @param device 
/// @param numIndex 
/// @param maxBufferCount 
void Detail::IndexBufferResource::Initialize(BufferAllocator* allocator, ID3D12Device* device, uint32_t numIndex, uint32_t maxBufferCount)
{
    assert(allocator);
    assert(device);
    assert(numIndex > 0);

    allocator_ = allocator;
    numIndex_ = numIndex;
    maxBufferCount_ = maxBufferCount > 1 ? static_cast<int32_t>(maxBufferCount) : 1;

    // 最大バッファ数に応じてサブアロケーション領域を確保
    regions_.resize(maxBufferCount_);
    view_.resize(maxBufferCount_);

    // リソースステートの初期化
    InitializeStates(maxBufferCount_, D3D12_RESOURCE_STATE_GENERIC_READ);

    uint64_t size = sizeof(uint32_t) * numIndex;

    for (int i = 0; i < maxBufferCount_; ++i)
    {
        // アロケータから領域を確保 (アライメントは4バイト)
        regions_[i] = allocator_->Allocate(size, sizeof(uint32_t));

        // ビューの設定
        view_[i].BufferLocation = regions_[i].parentBuffer->GetResource()->GetGPUVirtualAddress() + regions_[i].allocation.offset;
        view_[i].Format = DXGI_FORMAT_R32_UINT;
        view_[i].SizeInBytes = static_cast<UINT>(size);
    }
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param frameIndex
void Detail::IndexBufferResource::Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	assert(commandList);

	// フレームインデックスを最大バッファ数でラップする
	frameIndex = static_cast<int32_t>(frameIndex) < maxBufferCount_ ? frameIndex : 0;

	// インデックスバッファを設定する
	commandList->IASetIndexBuffer(&view_[frameIndex]);
}

/// @brief リソースを取得する
/// @param bufferIndex 
/// @return 
ID3D12Resource* Detail::IndexBufferResource::Get(uint32_t bufferIndex) const
{
    // バッファインデックスが範囲内であることを確認する
    assert(bufferIndex < regions_.size());

    // サブアロケーションされた領域の親バッファからリソースを取得する
    return regions_[bufferIndex].parentBuffer->GetResource();
}

/// @brief サブアロケートされた領域を取得する
/// @param bufferIndex 
/// @return 
const Detail::SubAllocatedRegion& Detail::IndexBufferResource::GetRegion(uint32_t bufferIndex) const
{
    assert(bufferIndex < regions_.size());
    return regions_[bufferIndex];
}
