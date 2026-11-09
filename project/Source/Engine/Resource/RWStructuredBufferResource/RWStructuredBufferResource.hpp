#pragma once
#include "IRWStructuredBufferResource.hpp"

namespace Detail
{
	/// @brief RWストラクチャードバッファリソース
	/// @tparam T 
	template<typename T>
	class RWStructuredBufferResource : public IRWStructuredBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param commandList 
		/// @param heap 
		/// @param num 
		/// @param maxBufferCount 
		RWStructuredBufferResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXHeap* heap,
            PlacedResourceAllocator* allocator , UINT num, uint32_t maxBufferCount) { Initialize(device, commandList, heap, allocator, num, maxBufferCount); }

		/// @brief デストラクタ
		~RWStructuredBufferResource();

		/// @brief リサイズ
		/// @param device 
		/// @param commandList 
		/// @param num 
		void Resize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num) override;


	private:

		/// @brief 初期化
		/// @param device 
		/// @param heap 
		/// @param num 
		/// @param maxBufferCount 
		void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXHeap* heap, PlacedResourceAllocator* allocator, UINT num, uint32_t maxBufferCount);
	};
}

/// @brief デストラクタ
template<typename T>
Detail::RWStructuredBufferResource<T>::~RWStructuredBufferResource()
{
	if (heap_)
	{
		for (int i = 0; i < resource_.size(); ++i)
		{
			// SRVハンドルを解放する
			heap_->FreeSrvDescriptorHandle(srvHandle_[i]);

			// UAVハンドルを解放する
			heap_->FreeSrvDescriptorHandle(uavHandle_[i]);
		}
	}

    // PlacedResource用のメモリをアロケータに返却
    if (allocator_)
    {
        for (auto& alloc : allocations_)
        {
            allocator_->Free(alloc);
        }
    }
}

/// @brief 初期化
/// @param device 
/// @param heap 
/// @param num 
/// @param maxBufferCount 
template<typename T>
void Detail::RWStructuredBufferResource<T>::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXHeap* heap, PlacedResourceAllocator* allocator, UINT num, uint32_t maxBufferCount)
{
	// nullptrチェック
	assert(device);
	assert(heap);
	assert(commandList);
    assert(allocator);
	assert(num > 0);


	// ヒープを保持する
	heap_ = heap;
    allocator_ = allocator;

	// 最大バッファ数を取得する
	if (maxBufferCount > maxBufferCount_) maxBufferCount_ = maxBufferCount;

	// リソースとデータの配列をリサイズする
	resource_.resize(maxBufferCount_);
	uavHandle_.resize(maxBufferCount_);
	srvHandle_.resize(maxBufferCount_);
    allocations_.resize(maxBufferCount_);

    // リソースステートの初期化
    InitializeStates(maxBufferCount_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);


    // バッファサイズを計算する
    uint64_t bufferSize = sizeof(T) * num;

	for (uint32_t i = 0; i < maxBufferCount_; ++i)
	{
        // メモリ領域の確保（64KBアライメント）
        allocations_[i] = allocator->Allocate(bufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        // リソースの設定 (バッファ)
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        desc.Width = bufferSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        // PlacedResource の作成
        HRESULT hr = device->CreatePlacedResource(allocator->GetHeap(allocations_[i].heapIndex), allocations_[i].offset, &desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resource_[i]));

        if (FAILED(hr))
        {
            throw std::runtime_error("CreatePlacedResource に失敗しました");
        }


		// UAVの設定
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = num;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		uavDesc.Buffer.StructureByteStride = sizeof(T);

		// ハンドルを取得する
		uavHandle_[i] = heap->GetSrvDescriptorHandle();

		// ビューの生成
		device->CreateUnorderedAccessView(resource_[i].Get(), nullptr, &uavDesc, uavHandle_[i].cpuHandle);


		// SRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = num;
		srvDesc.Buffer.StructureByteStride = sizeof(T);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// ハンドルを取得する
		srvHandle_[i] = heap->GetSrvDescriptorHandle();

		// ビューの生成
		device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
	}
}

/// @brief リサイズ
/// @param device 
/// @param commandList 
/// @param num 
template<typename T>
void Detail::RWStructuredBufferResource<T>::Resize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num)
{
    assert(device);
    assert(commandList);
    assert(num > 0);

    // バッファサイズを計算する
    uint64_t bufferSize = sizeof(T) * num;

    // リソースの設定 (バッファ)
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width = bufferSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        // 既存のリソースとメモリを解放
        resource_[i].Reset();
        if (allocator_) allocator_->Free(allocations_[i]);

        // 新しいサイズでメモリ再確保
        allocations_[i] = allocator_->Allocate(bufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        // PlacedResource の作成
        HRESULT hr = device->CreatePlacedResource(allocator_->GetHeap(allocations_[i].heapIndex), allocations_[i].offset, &desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resource_[i]));

        if (FAILED(hr)) throw std::runtime_error("RWStructuredBufferResource: リサイズ時の CreatePlacedResource に失敗");

        // UAVの再設定
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = num;
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        uavDesc.Buffer.StructureByteStride = sizeof(T);
        device->CreateUnorderedAccessView(resource_[i].Get(), nullptr, &uavDesc, uavHandle_[i].cpuHandle);

        // SRVの再設定
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = num;
        srvDesc.Buffer.StructureByteStride = sizeof(T);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
    }
}
