#pragma once
#include <vector>
#include <cassert>

#include "Func/ResourceFunc/ResourceFunc.h"
#include "RenderContext/DXHeap/DXHeap.h"

namespace Detail
{
	class IStructuredBufferResource
	{
	public:

		/// @brief コンストラクタ
		IStructuredBufferResource() = default;

		/// @brief 仮想デストラクタ
		virtual ~IStructuredBufferResource() = default;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex 
		virtual void RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex) = 0;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex 
		virtual void RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex) = 0;


	protected:

		/// @brief ヒープ
		DXHeap* heap_ = nullptr;

		/// @brief リソース
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;

		/// @brief SRVハンドル
		std::vector<SRVDescriptorHandle> handle_;
	};

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
		StructuredBufferResource(ID3D12Device* device, DXHeap* heap, UINT num, uint32_t frameIndex) { Initialize(device, heap, num, frameIndex); }

		/// @brief デストラクタ
		~StructuredBufferResource();

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex
		void RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex) override;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex
		void RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex) override;

		/// @brief データを取得する
		/// @param frameIndex 
		/// @return 
		T* GetData(uint32_t frameIndex) { return data_[frameIndex]; }


	private:

		/// @brief 初期化
		/// @param device 
		/// @param heap 
		/// @param num 
		/// @param frameIndex
		void Initialize(ID3D12Device* device, DXHeap* heap, UINT num, uint32_t frameIndex);

		/// @brief データ
		std::vector<T*> data_;
	};
}

/// @brief デストラクタ
template <typename T>
Detail::StructuredBufferResource<T>::~StructuredBufferResource()
{
	if (heap_)
	{
		for (int i = 0; i < resource_.size(); ++i)
		{
			// SRVハンドルを解放する
			heap_->FreeSrvDescriptorHandle(handle_[i]);
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
void Detail::StructuredBufferResource<T>::Initialize(ID3D12Device* device, DXHeap* heap, UINT num, uint32_t frameIndex)
{
	// nullptrチェック
	assert(device);
	assert(heap);
	assert(num > 0);

	// ヒープを保存する
	heap_ = heap;

	// 最大バッファ数を取得する
	uint32_t maxBufferCount = 1;
	if (frameIndex > maxBufferCount) maxBufferCount = frameIndex;
	
	// リソースとデータの配列をリサイズする
	resource_.resize(maxBufferCount);
	data_.resize(maxBufferCount);
	handle_.resize(maxBufferCount);

	for (uint32_t i = 0; i < maxBufferCount; ++i)
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
		handle_[i] = heap->GetSrvDescriptorHandle();

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