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
		VertexBufferResource(ID3D12Device* device, UINT numVertex, uint32_t maxBufferCount) { Initialize(device, numVertex, maxBufferCount); }

		/// @brief デストラクタ
		~VertexBufferResource() = default;

		/// @brief データを取得する
		/// @param frameIndex 
		/// @return 
		T* GetData(uint32_t frameIndex) { return data_[frameIndex]; }


	private:

		/// @brief 初期化
		/// @param device 
		/// @param numVertex 
		/// @param log 
		void Initialize(ID3D12Device* device, UINT numVertex, uint32_t maxBufferCount);

		/// @brief データ
		std::vector<T*> data_;
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

	// 頂点数を設定する
	numVertex_ = numVertex;

	// バッファ数を設定する
	maxBufferCount_ = maxBufferCount > 1 ? maxBufferCount : 1;

	// データ、リソース、ビューの配列を確保する
	data_.resize(maxBufferCount_);
	resource_.resize(maxBufferCount_);
	view_.resize(maxBufferCount_);

	for (uint32_t i = 0; i < maxBufferCount_; ++i)
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
