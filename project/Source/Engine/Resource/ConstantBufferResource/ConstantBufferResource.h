#pragma once
#include <vector>
#include <cassert>
#include <typeindex>

#include "Func/ResourceFunc/ResourceFunc.h"
#include "RenderContext/DXHeap/DXHeap.h"

namespace Detail
{
	class IConstantBufferResource
	{
	public:

		/// @brief コンストラクタ
		IConstantBufferResource() = default;

		/// @brief 仮想デストラクタ
		virtual ~IConstantBufferResource() = default;

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

		/// @brief GPU仮想アドレスを取得する
		/// @param frameIndex 
		/// @return 
		virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t frameIndex) const = 0;

		/// @brief 型情報を取得する
		/// @return 
		virtual std::type_index GetTypeIndex() const = 0;


	protected:

		/// @brief リソース配列
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;
	};

	/// @brief 定数バッファ
	/// @tparam T 
	template<typename T>
	class ConstantBufferResource : public IConstantBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		ConstantBufferResource(ID3D12Device* device, int32_t maxBuffer = 1) { Initialize(device, maxBuffer); }

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

		/// @brief 型情報を取得する
		/// @return 
		std::type_index GetTypeIndex() const override { return typeid(T); }

		/// @brief GPU仮想アドレスを取得する
		/// @param frameIndex 
		/// @return 
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t frameIndex) const override{ return resource_[frameIndex]->GetGPUVirtualAddress(); }


	private:

		/// @brief 初期化
		/// @param device 
		void Initialize(ID3D12Device* device, int32_t maxBuffer);

		/// @brief データ配列
		std::vector<T*> data_{ nullptr, nullptr };
	};
}

/// @brief 初期化
/// @param device 
/// @param log 
template<typename T>
void Detail::ConstantBufferResource<T>::Initialize(ID3D12Device* device, int32_t maxBuffer)
{
	// nullptrチェック
	assert(device);


	// 最大バッファ数を取得する
	int maxBufferCount = 1;
	if (maxBuffer > 0) maxBufferCount = maxBuffer;

	// リソースとデータの配列をリサイズする
	resource_.resize(maxBufferCount);
	data_.resize(maxBufferCount);

	// リソースとデータの割り当て
	for (int32_t i = 0; i < maxBufferCount; ++i)
	{
		// リソース生成
		resource_[i] = CreateBufferResource(device, sizeof(T));

		// データ割り当て
		resource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&data_[i]));
	}
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
template<typename T>
void Detail::ConstantBufferResource<T>::RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, resource_[frameIndex]->GetGPUVirtualAddress());
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
template<typename T>
void Detail::ConstantBufferResource<T>::RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootConstantBufferView(rootParameterIndex, resource_[frameIndex]->GetGPUVirtualAddress());
}