#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <vector>
#include <wrl.h>

namespace Detail
{
	/// @brief インデックスバッファリソース
	class IndexBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param numIndex 
		/// @param maxBufferCount 
		IndexBufferResource(ID3D12Device* device, uint32_t numIndex, uint32_t maxBufferCount) { Initialize(device, numIndex, maxBufferCount); }

		/// @brief デストラクタ
		~IndexBufferResource() = default;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param frameIndex
		void Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);

		/// @brief データを取得する
		/// @param frameIndex 
		/// @return 
		uint32_t* GetData(uint32_t frameIndex) { return data_[frameIndex]; }

		/// @brief インデックス数を取得する
		/// @return 
		UINT GetNumIndex() const { return numIndex_; }


		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

	private:

		/// @brief 初期化
		/// @param device 
		/// @param numIndex 
		/// @param maxBufferCount 
		void Initialize(ID3D12Device* device, uint32_t numIndex, uint32_t maxBufferCount);

		/// @brief 最大バッファ数
		int32_t maxBufferCount_ = 1;

		/// @brief インデックス数
		UINT numIndex_ = 0;

		// データ
		std::vector<uint32_t*> data_;

		/// @brief リソース
		std::vector<ComPtr<ID3D12Resource>> resource_;

		/// @brief ビュー
		std::vector<D3D12_INDEX_BUFFER_VIEW> view_;
	};
}