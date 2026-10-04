#include "MultiPass.h"
#include "Engine.h"

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

	// スワップチェーンのサイズを取得
	int width = static_cast<int32_t>(swapChain->GetSwapChainDesc().Width);
	int height = static_cast<int32_t>(swapChain->GetSwapChainDesc().Height);

	depthResource_ = std::make_unique<DepthResource>();
	depthResource_->Initialize(device, width, height, heap);

	// スワップチェーンコピーPSOを作成
	swapChainCopyPSO_ = std::make_unique<SwapChainCopyPSO>(device, shaderCompiler);

	// レンダーターゲットプールを作成
	renderTargetPool_ = std::make_unique<RenderTargetPool>(device, heap, swapChain, commandList);

	// レンダーパスシステムを作成
	renderPassSystem_ = std::make_unique<RenderPassSystem>(renderTargetPool_.get());
}

/// @brief クリア
/// @param commandList 
/// @param frameIndex
void Detail::MultiPass::Clear(ID3D12GraphicsCommandList* commandList, int frameIndex)
{
	assert(commandList);

	// デプスステンシルのクリア
	depthResource_->ClearDepthStencil(commandList, frameIndex);
}

/// @brief フレーム終了時の処理
/// @param commandList 
/// @param frameIndex
void Detail::MultiPass::EndFrame(ID3D12GraphicsCommandList* commandList, int frameIndex)
{
	renderPassSystem_->Return();
	sourceResource_ = nullptr;
}

/// @brief スワップチェインのRTVリソースにオフクリーンリソースを書き込む
/// @param commandList 
/// @param frameIndex
void Detail::MultiPass::RenderSwapChain(ID3D12GraphicsCommandList* commandList, int frameIndex)
{
	// リソースが存在しない場合は処理を行わない
	if (!currentResource_)return;

	// nullptrチェック
	assert(commandList);

	// PSOの設定
	swapChainCopyPSO_->Register(commandList);

	// テクスチャ
	currentResource_->RegisterGraphics(commandList, 0, frameIndex);

	// 形状は三角形
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点は3つ
	commandList->DrawInstanced(3, 1, 0, 0);
}

/// @brief レンダーパスを実行する
/// @param commandList 
void Detail::MultiPass::Execute(ID3D12GraphicsCommandList* commandList, int frameIndex)
{
	// nullptrチェック
	assert(commandList);

	// レンダーパスを実行する
	renderPassSystem_->ExecuteAll(commandList, this, depthResource_->GetDsvHandle(frameIndex));
}