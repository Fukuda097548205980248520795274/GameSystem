#pragma once
#include "IConstantBufferResource.h"

namespace Detail
{

	/// @brief 定数バッファリソース
	/// @tparam T 
	template<typename T>
	class ConstantBufferResource : public IConstantBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		ConstantBufferResource(ID3D12Device* device, uint32_t maxBuffer = 1) { Initialize(device, maxBuffer); }

		/// @brief データを取得する
		/// @param frameIndex 
		/// @return 
		T* GetData(uint32_t frameIndex) { return data_[frameIndex]; }

		/// @brief 型情報を取得する
		/// @return 
		std::type_index GetTypeIndex() const override { return typeid(T); }


	private:

		/// @brief 初期化
		/// @param device 
		void Initialize(ID3D12Device* device, uint32_t maxBuffer);

		/// @brief データ配列
		std::vector<T*> data_{ nullptr, nullptr };
	};
}

/// @brief 初期化
/// @param device 
/// @param log 
template<typename T>
void Detail::ConstantBufferResource<T>::Initialize(ID3D12Device* device, uint32_t maxBuffer)
{
	// nullptrチェック
	assert(device);


	// 最大バッファ数を取得する
	if (maxBuffer > maxBufferCount_) maxBufferCount_ = maxBuffer;

	// リソースとデータの配列をリサイズする
	resource_.resize(maxBufferCount_);
	data_.resize(maxBufferCount_);

	// リソースとデータの割り当て
	for (uint32_t i = 0; i < maxBufferCount_; ++i)
	{
		// リソース生成
		resource_[i] = CreateBufferResource(device, sizeof(T));

		// データ割り当て
		resource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&data_[i]));
	}
}