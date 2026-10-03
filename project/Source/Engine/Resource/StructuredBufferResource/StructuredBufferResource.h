#pragma once
#include <vector>
#include <cassert>

#include "Func/ResourceFunc/ResourceFunc.h"
#include "Engine.h"

namespace Detail
{
	/// @brief 構造化バッファリソース
	/// @tparam T 
	template<typename T>
	class StructuredBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param heap 
		/// @param num 
		StructuredBufferResource(ID3D12Device* device, DXHeap* heap, UINT num) { Initialize(device, heap, num); }

		/// @brief デストラクタ
		~StructuredBufferResource() = default;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex
		void RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex
		void RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief データを取得する
		/// @param frameIndex 
		/// @return 
		T* GetData(uint32_t frameIndex) { return data_[frameIndex]; }


	private:

		/// @brief 初期化
		/// @param device 
		/// @param heap 
		/// @param num 
		void Initialize(ID3D12Device* device, DXHeap* heap, UINT num);

		/// @brief リソース
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;

		/// @brief データ
		std::vector<T*> data_ = nullptr;

		/// @brief SRVハンドル
		std::vector<SRVDescriptorHandle> handle_;
	};
}

/// @brief 初期化
/// @param device 
/// @param heap 
/// @param num 
/// @param log 
template <typename T>
void Detail::StructuredBufferResource<T>::Initialize(ID3D12Device* device, DXHeap* heap, UINT num)
{
	// nullptrチェック
	assert(device);
	assert(heap);
	assert(num > 0);

	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	// 最大バッファ数を取得する
	int maxBufferCount = 1;
	if (engine) maxBufferCount = engine->GetMaxBufferCount();

	// リソースとデータの配列をリサイズする
	resource_.resize(maxBufferCount);
	data_.resize(maxBufferCount);
	handle_.resize(maxBufferCount);

	for (int i = 0; i < maxBufferCount; ++i)
	{
		// リソース作成
		resource_[i] = CreateBufferResource(device, sizeof(T) * num);

		// データを割りあてる
		resource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&data_[i]));

		// SRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = num;
		srvDesc.Buffer.StructureByteStride = sizeof(T);

		// ハンドルを取得する
		handle_[i].cpuHandle = heap->GetSrvDescriptorHandle().cpuHandle;
		handle_[i].gpuHandle = heap->GetSrvDescriptorHandle().gpuHandle;

		// ビューの生成
		device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, handle_[i].cpuHandle);
	}
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
template <typename T>
void Detail::StructuredBufferResource<T>::RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, handle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
template <typename T>
void Detail::StructuredBufferResource<T>::RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, handle_[frameIndex].gpuHandle);
}