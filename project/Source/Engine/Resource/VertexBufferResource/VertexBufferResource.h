#pragma once
#include <cassert>
#include <vector>
#include "Engine.h"
#include "Func/ResourceFunc/ResourceFunc.h"

namespace Detail
{
	template<typename T>
	class VertexBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param numVertex 
		/// @param maxBufferCount 
		VertexBufferResource(ID3D12Device* device, UINT numVertex, uint32_t maxBufferCount) { Initialize(device, numVertex, maxBufferCount); }

		/// @brief デストラクタ
		~VertexBufferResource() = default;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param frameIndex 
		void Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);

		/// @brief データを取得する
		/// @param frameIndex 
		/// @return 
		T* GetData(uint32_t frameIndex) { return data_[frameIndex]; }


		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

	private:

		/// @brief 初期化
		/// @param device 
		/// @param numVertex 
		/// @param log 
		void Initialize(ID3D12Device* device, UINT numVertex, uint32_t maxBufferCount);

		/// @brief 最大バッファ数
		int32_t maxBufferCount_ = 1;

		/// @brief データ
		std::vector<T*> data_;

		/// @brief リソース
		std::vector<ComPtr<ID3D12Resource>> resource_;

		/// @brief ビュー
		std::vector<D3D12_VERTEX_BUFFER_VIEW> view_;
	};
}

/// @brief 初期化
/// @param device 
/// @param numVertex 
/// @param maxBufferCount 
template<typename T>
void Detail::VertexBufferResource<T>::Initialize(ID3D12Device* device, UINT numVertex, uint32_t maxBufferCount)
{
	// nullptrチェック
	assert(device);
	assert(numVertex > 0);

	// バッファ数を設定する
	maxBufferCount_ = maxBufferCount_ > 1 ? static_cast<int32_t>(maxBufferCount_) : 1;

	// データ、リソース、ビューの配列を確保する
	data_.resize(maxBufferCount_);
	resource_.resize(maxBufferCount_);
	view_.resize(maxBufferCount_);

	for (int i = 0; i < maxBufferCount_; ++i)
	{
		// リソース生成
		resource_[i] = CreateBufferResource(device, sizeof(T) * numVertex);
		resource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&data_[i]));

		// ビューの割り当て
		view_[i].BufferLocation = resource_[i]->GetGPUVirtualAddress();
		view_[i].SizeInBytes = sizeof(T) * numVertex;
		view_[i].StrideInBytes = sizeof(T);
	}
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param frameIndex 
template<typename T>
void Detail::VertexBufferResource<T>::Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	assert(commandList);

	// フレームインデックスを最大バッファ数でラップする
	frameIndex = static_cast<int32_t>(frameIndex) < maxBufferCount_ ? frameIndex : 0;
	
	// ビューをコマンドリストに設定する
	commandList->IASetVertexBuffers(0, 1, &view_[frameIndex]);
}