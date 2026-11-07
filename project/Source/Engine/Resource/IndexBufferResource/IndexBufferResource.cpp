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

    regions_.resize(maxBufferCount_);
    view_.resize(maxBufferCount_);

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
