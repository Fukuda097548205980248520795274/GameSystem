#include "MultiPass.h"

/// @brief 初期化
/// @param device 
/// @param heap 
/// @param swapChain 
/// @param commandList 
void Detail::MultiPass::Initialize(ID3D12Device* device, DXHeap* heap, DXSwapChain* swapChain, ID3D12GraphicsCommandList* commandList, ShaderCompiler* shaderCompiler)
{
	// nullptrチェック
	assert(device);
	assert(heap);
	assert(swapChain);
	assert(commandList);
	assert(shaderCompiler);

	// 引数を受け取る
	heap_ = heap;
	buffering_ = swapChain;

	// スワップチェーンコピーPSOを作成
	swapChainCopyPSO_ = std::make_unique<SwapChainCopyPSO>(device, shaderCompiler);

	// レンダーターゲットプールを作成
	renderTargetPool_ = std::make_unique<RenderTargetPool>(device, heap, swapChain, commandList);
}