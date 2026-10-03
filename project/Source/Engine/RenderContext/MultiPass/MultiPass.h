#pragma once
#include "RenderTargetPool/RenderTargetPool.h"
#include "SwapChainCopyPSO/SwapChainCopyPSO.h"

namespace Detail
{
	class ShaderCompiler;

	class MultiPass
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param heap 
		/// @param swapChain 
		/// @param commandList 
		/// @param shaderCompiler 
		MultiPass(ID3D12Device* device, DXHeap* heap, DXSwapChain* swapChain, ID3D12GraphicsCommandList* commandList, ShaderCompiler* shaderCompiler) 
		{
			Initialize(device, heap, swapChain, commandList, shaderCompiler); 
		}

		/// @brief デストラクタ
		~MultiPass() = default;


	private:

		/// @brief 初期化
		/// @param device 
		/// @param heap 
		/// @param swapChain 
		/// @param commandList 
		/// @param shaderCompiler 
		void Initialize(ID3D12Device* device, DXHeap* heap, DXSwapChain* swapChain, ID3D12GraphicsCommandList* commandList, ShaderCompiler* shaderCompiler);


	private:

		/// @brief レンダーターゲットプール
		std::unique_ptr<RenderTargetPool> renderTargetPool_;

		/// @brief スワップチェーンコピーPSO
		std::unique_ptr<SwapChainCopyPSO> swapChainCopyPSO_;


	private:

		/// @brief ヒープ
		DXHeap* heap_;

		/// @brief DXSwapChain
		DXSwapChain* buffering_;
	};
}