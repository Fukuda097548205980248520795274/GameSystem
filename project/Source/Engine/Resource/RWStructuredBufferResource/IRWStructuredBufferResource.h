#pragma once
#include <vector>
#include <cassert>
#include <typeindex>

#include "Func/ResourceFunc/ResourceFunc.h"
#include "Func/Barrier/Barrier.h"
#include "RenderContext/DXHeap/DXHeap.h"

namespace Detail
{
	/// @brief RWストラクチャードバッファリソース : インタフェース
	class IRWStructuredBufferResource
	{
	public:

		/// @brief コンストラクタ
		IRWStructuredBufferResource() = default;

		/// @brief 仮想デストラクタ
		virtual ~IRWStructuredBufferResource() = default;

		/// @brief リサイズ
		/// @param device 
		/// @param commandList 
		/// @param num 
		virtual void Resize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num) = 0;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		void RegisterComputeUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		void RegisterComputeSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		void RegisterGraphicsSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief バリアを張る
		/// @param commandList 
		/// @param before 
		/// @param after 
		void Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex);

		/// @brief 全てのバリアを張る
		/// @param commandList 
		/// @param before 
		/// @param after 
		void AllBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);


	protected:

		/// @brief ヒープ
		DXHeap* heap_ = nullptr;

		/// @brief 最大バッファ数
		uint32_t maxBufferCount_ = 1;

		/// @brief リソース
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;

		/// @brief UAVハンドル
		std::vector<SRVDescriptorHandle> uavHandle_;

		/// @brief SRVハンドル
		std::vector<SRVDescriptorHandle> srvHandle_;
	};
}