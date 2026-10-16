#pragma once
#include "IRWStructuredBufferResource.h"

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
		RWStructuredBufferResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXHeap* heap, UINT num, uint32_t maxBufferCount) { Initialize(device, commandList, heap, num, maxBufferCount); }

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
		void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXHeap* heap, UINT num, uint32_t maxBufferCount);
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
}

/// @brief 初期化
/// @param device 
/// @param heap 
/// @param num 
/// @param maxBufferCount 
template<typename T>
void Detail::RWStructuredBufferResource<T>::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXHeap* heap, UINT num, uint32_t maxBufferCount)
{
	// nullptrチェック
	assert(device);
	assert(heap);
	assert(commandList);
	assert(num > 0);


	// ヒープを保持する
	heap_ = heap;

	// 最大バッファ数を取得する
	if (maxBufferCount > maxBufferCount_) maxBufferCount_ = maxBufferCount;

	// リソースとデータの配列をリサイズする
	resource_.resize(maxBufferCount_);
	uavHandle_.resize(maxBufferCount_);
	srvHandle_.resize(maxBufferCount_);

	for (uint32_t i = 0; i < maxBufferCount_; ++i)
	{
		// リソース作成
		resource_[i] = CreateUAVResource(device, sizeof(T) * num, commandList);


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
	// nullptrチェック
	assert(device);
	assert(commandList);
	assert(num > 0);

	for (uint32_t i = 0; i < maxBufferCount_; ++i)
	{
		// 既存のリソースを解放する
		resource_[i].Reset();

		// リソース作成
		resource_[i] = CreateUAVResource(device, sizeof(T) * num, commandList);


		// UAVの設定
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = num;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		uavDesc.Buffer.StructureByteStride = sizeof(T);

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

		// ビューの生成
		device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
	}
}