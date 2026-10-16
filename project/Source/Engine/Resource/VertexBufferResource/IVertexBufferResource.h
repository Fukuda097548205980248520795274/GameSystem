#pragma once
#include <cassert>
#include <vector>
#include "Func/ResourceFunc/ResourceFunc.h"

namespace Detail
{
	/// @brief 頂点バッファリソース : インタフェース
	class IVertexBufferResource
	{
	public:

		/// @brief コンストラクタ
		IVertexBufferResource() = default;

		/// @brief 仮想デストラクタ
		virtual ~IVertexBufferResource() = default;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param frameIndex 
		void Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);

		/// @brief 頂点数を取得する
		/// @return 
		UINT GetNumVertex() const { return numVertex_; }


		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;


	protected:

		/// @brief 最大バッファ数
		uint32_t maxBufferCount_ = 1;

		/// @brief 頂点数
		UINT numVertex_ = 0;

		/// @brief リソース
		std::vector<ComPtr<ID3D12Resource>> resource_;

		/// @brief ビュー
		std::vector<D3D12_VERTEX_BUFFER_VIEW> view_;
	};
}