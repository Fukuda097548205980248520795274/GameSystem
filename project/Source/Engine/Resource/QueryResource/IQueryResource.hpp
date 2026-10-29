#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <cstdint>

namespace Detail
{
	/// @brief DXヒープ
	class DXHeap;

	/// @brief 基底クエリリソース
	class IQueryResource
	{
	public:

		/// @brief デストラクタ
		IQueryResource() = default;

		/// @brief 仮想デストラクタ
		virtual ~IQueryResource() = default;

		/// @brief クエリを記録する
		/// @param commandList 
		void Record(ID3D12GraphicsCommandList* commandList);

		/// @brief クエリ結果を解決（リードバックバッファへコピー）する
		/// @param commandList 
		/// @param frameIndex 
		void Resolve(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);

		/// @brief クエリのインデックスを取得する
		/// @return 
		uint32_t GetQueryIndex() const { return queryIndex_; }


	protected:

		/// @brief リードバックリソース配列
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> readbackBuffers_;

		/// @brief 最大バッファ数
		uint32_t maxBufferCount_ = 1;

		/// @brief クエリインデックス
		uint32_t queryIndex_ = 0;

		/// @brief DXHeapへのポインタ
		DXHeap* heap_ = nullptr;
	};
}