#pragma once
#include <utility>
#include <vector>

#include "RenderContext/DXHeap/DXHeap.h"

namespace Detail
{
	class DXHeap;

	class DepthResource
	{
	public:

		/// @brief 初期化
		/// @param device 
		/// @param width 
		/// @param height 
		/// @param heap 
		DepthResource(ID3D12Device* device, int32_t width, int32_t height, DXHeap* heap) { Initialize(device, width, height, heap); }

		/// @brief デストラクタ
		~DepthResource();

		/// @brief サイズを作り直す
		/// @param device 
		/// @param width 
		/// @param height 
		void Resize(ID3D12Device* device, int32_t width, int32_t height);

		/// @brief バリアを張る
		/// @param commandList 
		/// @param before 
		/// @param after 
		/// @param frameIndex
		void Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex);

		/// @brief デプスステンシルのクリア
		/// @param commandList 
		/// @param frameIndex
		void ClearDepthStencil(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);

		/// @brief DSV用ハンドルを取得する
		/// @param frameIndex
		/// @return 
		D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle(uint32_t frameIndex) { return dsvHandle_[frameIndex]; }

		/// @brief リソースを取得する
		/// @param frameIndex
		/// @return 
		ID3D12Resource* GetResource(uint32_t frameIndex) { return resource_[frameIndex].Get(); }

		/// @brief コマンドリストに登録する
		/// @param commandList
		/// @param rootParameterIndex
		/// @param frameIndex
		void RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief SRVをコマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex
		void RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief DSV用ハンドル（読み取り専用）を取得する
		/// @param frameIndex
		/// @return 
		D3D12_CPU_DESCRIPTOR_HANDLE GetDsvReadOnlyHandle(uint32_t frameIndex) { return dsvReadOnlyHandle_[frameIndex]; }


	private:

		/// @brief 初期化
		/// @param device 
		/// @param width 
		/// @param height 
		/// @param heap 
		void Initialize(ID3D12Device* device, int32_t width, int32_t height, DXHeap* heap);

		/// @brief ヒープ
		DXHeap* heap_ = nullptr;

		// リソース
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;

		/// @brief SRV用ハンドル
		std::vector<SRVDescriptorHandle> srvHandle_;

		// DSV用ハンドル
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dsvHandle_;

		/// @brief DSV用ハンドル（読み取り専用）
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dsvReadOnlyHandle_;
	};
}