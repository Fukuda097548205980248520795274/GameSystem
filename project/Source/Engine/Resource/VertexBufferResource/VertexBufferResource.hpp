#pragma once
#include "IVertexBufferResource.hpp"

namespace Detail
{
	/// @brief 頂点バッファリソース
	/// @tparam T 
	template<typename T>
	class VertexBufferResource : public IVertexBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param numVertex 
		/// @param maxBufferCount 
		VertexBufferResource(BufferAllocator* allocator, ID3D12Device* device, UINT numVertex, uint32_t maxBufferCount) { Initialize(allocator, device, numVertex, maxBufferCount); }

		/// @brief デストラクタ
		~VertexBufferResource();

		/// @brief データを取得する
		/// @param frameIndex 
		/// @return 
        T* GetData(uint32_t frameIndex) { return static_cast<T*>(regions_[frameIndex].cpuAddress); }


	private:

		/// @brief 初期化
		/// @param device 
		/// @param numVertex 
		/// @param log 
		void Initialize(BufferAllocator* allocator, ID3D12Device* device, UINT numVertex, uint32_t maxBufferCount);
	};
}

/// @brief デストラクタ
template<typename T>
Detail::VertexBufferResource<T>::~VertexBufferResource()
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
/// @param numVertex 
/// @param maxBufferCount 
template<typename T>
void Detail::VertexBufferResource<T>::Initialize(BufferAllocator* allocator, ID3D12Device* device, UINT numVertex, uint32_t maxBufferCount)
{
    assert(allocator);
    assert(device);
    assert(numVertex > 0);

    allocator_ = allocator;
    numVertex_ = numVertex;
    maxBufferCount_ = maxBufferCount > 1 ? maxBufferCount : 1;

    regions_.resize(maxBufferCount_);
    view_.resize(maxBufferCount_);

    uint64_t size = sizeof(T) * numVertex;

    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        // メモリ確保 (アライメントは要素サイズ)
        regions_[i] = allocator_->Allocate(size, sizeof(T));

        // ビューの割り当て（親バッファのGPUアドレスにオフセットを足す）
        view_[i].BufferLocation = regions_[i].parentBuffer->GetResource()->GetGPUVirtualAddress() + regions_[i].allocation.offset;
        view_[i].SizeInBytes = static_cast<UINT>(size);
        view_[i].StrideInBytes = sizeof(T);
    }
}
