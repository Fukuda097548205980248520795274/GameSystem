#pragma once
#include <vector>
#include <cassert>

#include "Func/ResourceFunc/ResourceFunc.h"
#include "Engine.h"

namespace Detail
{
	/// @brief 定数バッファ
	/// @tparam T 
	template<typename T>
	class ConstantBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		ConstantBufferResource(id3d12Device* device) { Initialize(device); }

		/// @brief デストラクタ
		~ConstantBufferResource();

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
		void Initialize(ID3D12Device* device);

		/// @brief リソース配列
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;

		/// @brief データ配列
		std::vector<T*> data_{ nullptr, nullptr };
	};
}


/// @brief 初期化
/// @param device 
/// @param log 
template<typename T>
void Detail::ConstantBufferResource<T>::Initialize(ID3D12Device* device)
{
	// nullptrチェック
	assert(device);

	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();


	// 最大バッファ数を取得する
	int maxBufferCount = 1;
	if (engine) maxBufferCount = engine->GetMaxBufferCount();

	// リソースとデータの配列をリサイズする
	resource_.resize(maxBufferCount);
	data_.resize(maxBufferCount);

	// リソースとデータの割り当て
	for (uint32_t i = 0; i < maxBufferCount; ++i)
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