#pragma once
#include <vector>
#include <cassert>
#include <typeindex>

#include "Func/ResourceFunc/ResourceFunc.hpp"
#include "RenderContext/DXHeap/DXHeap.hpp"

namespace Detail
{
	/// @brief 定数バッファリソース : インタフェース
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
		void RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex 
		void RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief GPU仮想アドレスを取得する
		/// @param frameIndex 
		/// @return 
		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t frameIndex) const { return resource_[frameIndex]->GetGPUVirtualAddress(); }

		/// @brief 型情報を取得する
		/// @return 
		virtual std::type_index GetTypeIndex() const = 0;


	protected:

		/// @brief リソース配列
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;

		/// @brief 最大バッファ数
		uint32_t maxBufferCount_ = 1;
	};

}