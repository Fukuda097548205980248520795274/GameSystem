#pragma once
#include <vector>
#include <cassert>
#include <typeindex>

#include "Func/ResourceFunc/ResourceFunc.h"
#include "Func/Barrier/Barrier.h"
#include "RenderContext/DXHeap/DXHeap.h"

namespace Detail
{
	class IRWStructuredBufferResource
	{
	public:

		/// @brief コンストラクタ
		IRWStructuredBufferResource() = default;

		/// @brief 仮想デストラクタ
		virtual ~IRWStructuredBufferResource() = default;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		virtual void RegisterComputeUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex) = 0;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		virtual void RegisterComputeSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex) = 0;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		virtual void RegisterGraphicsSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex) = 0;

		/// @brief バリアを張る
		/// @param commandList 
		/// @param before 
		/// @param after 
		virtual void Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex) = 0;

		/// @brief 全てのバリアを張る
		/// @param commandList 
		/// @param before 
		/// @param after 
		virtual void AllBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) = 0;


	protected:

		/// @brief ヒープ
		DXHeap* heap_ = nullptr;

		/// @brief リソース
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;

		/// @brief UAVハンドル
		std::vector<SRVDescriptorHandle> uavHandle_;

		/// @brief SRVハンドル
		std::vector<SRVDescriptorHandle> srvHandle_;
	};

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

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		void RegisterComputeUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex) override;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		void RegisterComputeSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex) override;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		void RegisterGraphicsSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex) override;

		/// @brief バリアを張る
		/// @param commandList 
		/// @param before 
		/// @param after 
		void Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex) override;

		/// @brief 全てのバリアを張る
		/// @param commandList 
		/// @param before 
		/// @param after 
		void AllBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) override;


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
	uint32_t bufferCount = 1;
	if (maxBufferCount > bufferCount) bufferCount = maxBufferCount;

	// リソースとデータの配列をリサイズする
	resource_.resize(bufferCount);
	uavHandle_.resize(bufferCount);
	srvHandle_.resize(bufferCount);

	for (uint32_t i = 0; i < bufferCount; ++i)
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

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
template<typename T>
void Detail::RWStructuredBufferResource<T>::RegisterComputeUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, uavHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
template<typename T>
void Detail::RWStructuredBufferResource<T>::RegisterComputeSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
template<typename T>
void Detail::RWStructuredBufferResource<T>::RegisterGraphicsSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief バリアを張る
/// @param commandList 
/// @param before 
/// @param after 
template<typename T>
void Detail::RWStructuredBufferResource<T>::Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex)
{
	TransitionBarrier(resource_[frameIndex].Get(), before, after, commandList);
}

/// @brief 全てのバリアを張る
/// @param commandList 
/// @param before 
/// @param after 
template<typename T>
void Detail::RWStructuredBufferResource<T>::AllBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	for (int i = 0; i < resource_.size(); ++i)
	{
		TransitionBarrier(resource_[i].Get(), before, after, commandList);
	}
}