#pragma once
#include <vector>
#include "RenderContext/DXHeap/DXHeap.h"

namespace Detail
{
	class RWTexture2DBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param commandList 
		/// @param heap 
		/// @param width 
		/// @param height 
		/// @param maxBufferCount 
		RWTexture2DBufferResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXHeap* heap, uint32_t width, uint32_t height, uint32_t maxBufferCount) { Initialize(device, commandList, heap, width, height, maxBufferCount); }

		/// @brief デストラクタ
		~RWTexture2DBufferResource();

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		void RegisterGraphicsSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		void RegisterGraphicsUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		void RegisterComputeUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		void RegisterComputeSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

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

		/// @brief 横幅を取得する
		/// @return 
		int32_t GetWidth()const { return width_; }

		/// @brief 縦幅を取得する
		/// @return 
		int32_t GetHeight()const { return height_; }

	private:

		/// @brief 初期化
		/// @param device 
		/// @param commandList 
		/// @param heap 
		/// @param width 
		/// @param height 
		/// @param maxBufferCount 
		void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXHeap* heap, uint32_t width, uint32_t height, uint32_t maxBufferCount);

		// @brief ヒープ
		DXHeap* heap_ = nullptr;

		/// @brief リソース
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;

		/// @brief SRVハンドル
		std::vector<SRVDescriptorHandle> srvHandle_;

		/// @brief UAVハンドル
		std::vector<SRVDescriptorHandle> uavHandle_;

		// @brief テクスチャの幅
		int32_t width_ = 0;
		int32_t height_ = 0;
	};
}