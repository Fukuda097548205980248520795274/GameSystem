#pragma once
#include <memory>
#include <vector>
#include <queue>
#include <cassert>
#include "Resource/OffscreenResource/OffscreenResource.h"

namespace Detail
{
	class DXHeap;
	class DXSwapChain;

	class RenderTargetPool
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param heap 
		/// @param swapChain 
		/// @param commandList 
		/// @param poolSize 
		RenderTargetPool(ID3D12Device* device, DXHeap* heap, DXSwapChain* swapChain, ID3D12GraphicsCommandList* commandList, int poolSize = 3) { Initialize(device, heap, swapChain, commandList, poolSize); }

		/// @brief デストラクタ
		~RenderTargetPool() = default;

		/// @brief 空いているレンダーターゲットを借りる
		/// @return 
		OffscreenResource* Rent(ID3D12GraphicsCommandList* commandList);

		/// @brief 使い終わったレンダーターゲットを返却する
		/// @param resource 
		void Return(OffscreenResource* resource);

		/// @brief フレーム終了時にリソースがすべて返却されているか確認する
		void CheckMemoryLeaks();

		/// @brief サイズを作り直す
		/// @param width 
		/// @param height 
		void Resize(int width, int height, ID3D12GraphicsCommandList* commandList);


	private:

		/// @brief 初期化
		/// @param device 
		/// @param heap 
		/// @param buffering 
		/// @param poolSize 
		/// @param commandList 
		void Initialize(ID3D12Device* device, DXHeap* heap, DXSwapChain* swapChain, ID3D12GraphicsCommandList* commandList, int poolSize);

		/// @brief レンダーターゲットを作成する
		/// @param width 
		/// @param height 
		void CreateRenderTarget(int width, int height, ID3D12GraphicsCommandList* commandList);


	private:

		// レンダーターゲットのリスト
		std::vector<std::unique_ptr<OffscreenResource>> resources_;

		// レンダーターゲットのプール
		std::queue<OffscreenResource*> freeQueue_;


		/// @brief デバイス
		ID3D12Device* device_ = nullptr;

		/// @brief ヒープ
		DXHeap* heap_ = nullptr;

		/// @brief バッファリング
		DXSwapChain* swapChain_ = nullptr;
	};
}