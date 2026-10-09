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

	// デプスリソースを作成
	depthResource_ = std::make_unique<DepthResource>(device, width, height, heap);

	// ダミーリソースを作成
	dummyResource_ = std::make_unique<OffscreenResource>(device, heap, 1, 1);
	dummyResource_->AllBarrier(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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

	// 初期に戻ったので、現在のリソースをクリア
	currentResource_ = nullptr;

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
void Detail::MultiPass::Execute(ID3D12GraphicsCommandList* commandList, RenderSystem* renderSystem, int frameIndex)
{
	// nullptrチェック
	assert(commandList);
	assert(renderSystem);
	assert(frameIndex >= 0);

	// レンダーパスを実行する
	renderPassSystem_->ExecuteAll(commandList, depthResource_->GetDsvHandle(frameIndex), this, renderSystem);

	// レンダーパスの結果が存在しない場合は、ダミーリソースを設定する
	if (!currentResource_)
	{
		currentResource_ = dummyResource_.get();
	}
}

/// @brief サイズを作り直す
/// @param device 
/// @param commandList 
/// @param width 
/// @param height 
void Detail::MultiPass::Resize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, int width, int height)
{
	// レンダーターゲットプールのリサイズ
	renderTargetPool_->Resize(width, height, commandList);

	// 深度リソースのリサイズ
	depthResource_->Resize(device, width, height);
}