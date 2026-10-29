#pragma once
#include <vector>
#include <cassert>


#include "Func/ResourceFunc/ResourceFunc.hpp"
#include "RenderContext/DXHeap/DXHeap.hpp"

namespace Detail
{
	/// @brief ストラクチャードバッファリソース : インタフェーズ
	class IStructuredBufferResource
	{
	public:

		/// @brief コンストラクタ
		IStructuredBufferResource() = default;

		/// @brief 仮想デストラクタ
		virtual ~IStructuredBufferResource() = default;

		/// @brief リサイズ
		/// @param device 
		/// @param num 
		virtual void Resize(ID3D12Device* device, UINT num) = 0;

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


	protected:

		/// @brief ヒープ
		DXHeap* heap_ = nullptr;

		/// @brief 最大バッファ数
		uint32_t maxBufferCount_ = 1;

		/// @brief リソース
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;

		/// @brief SRVハンドル
		std::vector<SRVDescriptorHandle> handle_;
	};
}