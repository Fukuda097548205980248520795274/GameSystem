#pragma once
#include "IStructuredBufferResource.hpp"

namespace Detail
{
	/// @brief 構造化バッファリソース
	/// @tparam T 
	template<typename T>
	class StructuredBufferResource : public IStructuredBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param heap 
		/// @param num 
		/// @param frameIndex 
		StructuredBufferResource(BufferAllocator* allocator, ID3D12Device* device, DXHeap* heap, UINT num, uint32_t frameIndex) { Initialize(allocator, device, heap, num, frameIndex); }

		/// @brief デストラクタ
		~StructuredBufferResource();

		/// @brief リサイズ
		/// @param device 
		/// @param num 
		void Resize(BufferAllocator* allocator, ID3D12Device* device, UINT num) override;

		/// @brief データを取得する
		/// @param frameIndex 
		/// @return 
        T* GetData(uint32_t frameIndex) { return static_cast<T*>(regions_[frameIndex].cpuAddress); }


	private:

		/// @brief 初期化
		/// @param device 
		/// @param heap 
		/// @param num 
		/// @param frameIndex
		void Initialize(BufferAllocator* allocator, ID3D12Device* device, DXHeap* heap, UINT num, uint32_t frameIndex);
	};
}

/// @brief デストラクタ
template <typename T>
Detail::StructuredBufferResource<T>::~StructuredBufferResource()
{
    // ヒープからSRVハンドルを解放する
    if (heap_)
    {
        for (int i = 0; i < handle_.size(); ++i)
        {
            heap_->FreeSrvDescriptorHandle(handle_[i]);
        }
    }

    // サブアロケータからメモリを解放する
    if (allocator_)
    {
        for (auto& region : regions_)
        {
            allocator_->Free(region);
        }
    }
}

/// @brief 初期化
/// @param device 
/// @param heap 
/// @param num 
/// @param log 
/// @param frameIndex
template <typename T>
void Detail::StructuredBufferResource<T>::Initialize(BufferAllocator* allocator, ID3D12Device* device, DXHeap* heap, UINT num, uint32_t frameIndex)
{
    assert(allocator);
    assert(device);
    assert(heap);
    assert(num > 0);

    allocator_ = allocator;
    heap_ = heap;
    maxBufferCount_ = std::max(maxBufferCount_, frameIndex);

    // サブアロケータからメモリを確保する
    regions_.resize(maxBufferCount_);
    handle_.resize(maxBufferCount_);

    // メモリ確保のサイズを計算する
    InitializeStates(maxBufferCount_, D3D12_RESOURCE_STATE_GENERIC_READ);

    // リサイズ前の要素数を保持する
    oldSize_ = num;

    // メモリ確保のサイズを計算する
    uint64_t size = sizeof(T) * num;

    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        // SRVのFirstElement制約のため、sizeof(T)でアライメントする
        regions_[i] = allocator_->Allocate(size, sizeof(T));

        // SRVの設定
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        // サブアロケーションのオフセットから、何要素目から開始するかを計算
        srvDesc.Buffer.FirstElement = regions_[i].allocation.offset / sizeof(T);
        srvDesc.Buffer.NumElements = num;
        srvDesc.Buffer.StructureByteStride = sizeof(T);

        // ビューの生成
        handle_[i] = heap->GetSrvDescriptorHandle();
        device->CreateShaderResourceView(regions_[i].parentBuffer->GetResource(), &srvDesc, handle_[i].cpuHandle);
    }
}

/// @brief リサイズ
/// @param device 
/// @param num 
template<typename T>
void Detail::StructuredBufferResource<T>::Resize(BufferAllocator* allocator, ID3D12Device* device, UINT num)
{
    assert(allocator);
    assert(device);
    assert(num > 0);

    uint64_t newSize = sizeof(T) * num;

    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        // 新しいサイズでアロケータから領域を確保する（要素サイズでアライメント）
        SubAllocatedRegion newRegion = allocator->Allocate(newSize, sizeof(T));

        // （必要であれば）古い領域から新しい領域へデータをコピーする
        if (regions_[i].IsValid() && regions_[i].cpuAddress && newRegion.cpuAddress)
        {
            memcpy(newRegion.cpuAddress, regions_[i].cpuAddress, oldSize_ * sizeof(T));
        }

        // 古い領域をアロケータに返却する
        if (regions_[i].IsValid())
        {
            allocator_->Free(regions_[i]);
        }

        // 領域情報を更新する
        regions_[i] = newRegion;

        // SRV（シェーダーリソースビュー）を作り直す
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        // オフセットから要素インデックスを算出
        srvDesc.Buffer.FirstElement = regions_[i].allocation.offset / sizeof(T);
        srvDesc.Buffer.NumElements = num;
        srvDesc.Buffer.StructureByteStride = sizeof(T);

        // 既存のハンドル（CPUディスクリプタ）に上書きでビューを生成
        device->CreateShaderResourceView(regions_[i].parentBuffer->GetResource(), &srvDesc, handle_[i].cpuHandle);
    }

    // リサイズ前の要素数を更新する
    oldSize_ = num;
}
