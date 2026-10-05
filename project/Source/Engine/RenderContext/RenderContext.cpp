#include "RenderContext.h"
#include <cassert>

#include "WinApp/WinApp.h"
#include "Func/Barrier/Barrier.h"

#include "DXDebug/DXDebug.h"

/// @brief 初期化
/// @param winApp 
/// @param dxDebug 
void Detail::RenderContext::Initialize(WinApp* winApp, DXDebug* dxDebug)
{
	// nullptrチェック
	assert(winApp);

	// 引数を受け取る
	winApp_ = winApp;

	// DXCoreを作成
	core_ = std::make_unique<DXCore>();
	if(dxDebug)dxDebug->Stop(core_->GetDevice());

	// DXCommandを作成
	command_ = std::make_unique<DXCommand>();
	command_->Initialize(core_->GetDevice());

	// DXFenceを作成
	fence_ = std::make_unique<DXFence>();
	fence_->Initialize(core_->GetDevice());

	// DXHeapを作成
	heap_ = std::make_unique<DXHeap>();
	heap_->Initialize(core_->GetDevice());

	// DXSwapChainを作成
	swapChain_ = std::make_unique<DXSwapChain>(heap_.get(), winApp, core_.get(), command_.get());

	// シェーダコンパイラを作成
	shaderCompiler_ = std::make_unique<ShaderCompiler>();
	shaderCompiler_->Initialize();

	// マルチパスを作成
	multiPass_ = std::make_unique<MultiPass>(core_->GetDevice(), heap_.get(), swapChain_.get(), command_->GetCommandList(), shaderCompiler_.get());

	// テクスチャストアを作成
	textureStore_ = std::make_unique<TextureStore>();


	// ビューポートの設定
	viewport_.Width = static_cast<float>(winApp->GetClientWidth());
	viewport_.Height = static_cast<float>(winApp->GetClientHeight());
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	// シザー矩形の設定
	scissorRect_.left = 0;
	scissorRect_.right = winApp->GetClientWidth();
	scissorRect_.top = 0;
	scissorRect_.bottom = winApp->GetClientHeight();


#ifdef DEVELOPMENT
	
	// ImGuiRenderを作成
	imguiRender_ = std::make_unique<ImGuiRender>();
	imguiRender_->Initialize(core_->GetDevice(), winApp, heap_.get(), swapChain_.get());

#endif

	// PSOEditorを作成
	psoEditor_ = std::make_unique<Detail::PSOEditor>();


	// 初期化時のコマンドリストを閉じる
	auto commandList = command_->GetCommandList();
	commandList->Close();

	// GPUにコマンドリストの実行を行わせる
	ID3D12CommandList* commandLists[] = { commandList };
	command_->GetCommandQueue()->ExecuteCommandLists(1, commandLists);

	// GPUにシグナルを送る
	fence_->SendSignal(command_->GetCommandQueue(), frameIndex_);

	// GPUの処理が完了するまで待機する
	fence_->WaitGPU(frameIndex_);

	// コマンドアロケータを取得
	auto commandAllocator = command_->GetCommandAllocator();

	// 次のフレーム用のコマンドリストを準備
	HRESULT hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator, nullptr);
	assert(SUCCEEDED(hr));
}

/// @brief デストラクタ
Detail::RenderContext::~RenderContext()
{
	// GPUの処理が完了するまで待機する
	if (command_ && fence_)
	{
		fence_->SendSignal(command_->GetCommandQueue(), frameIndex_);
		fence_->WaitGPU(frameIndex_);
	}
}

/// @brief シーン前処理
void Detail::RenderContext::NewFrame()
{
#ifdef DEVELOPMENT

	// フレームの開始をImGuiに伝える
	imguiRender_->FrameStart();

	// Dockスペースを作成する
	imguiRender_->CreateDockSpace();

	// PSOエディタのUIを描画する
	psoEditor_->DrawUI(core_->GetDevice(), shaderCompiler_.get());

#endif
}

/// @brief 描画後処理
void Detail::RenderContext::PostDraw()
{
	// コマンドリストを取得
	auto commandList = command_->GetCommandList();

	// GPUの処理が完了するまで待機する
	fence_->WaitGPU(frameIndex_);

	// 初回フレームでなければコマンドリストをリセットする
	if (!isFirstFrame_)
	{
		// コマンドアロケータを取得
		auto commandAllocator = command_->GetCommandAllocator();

		// 次のフレーム用のコマンドリストを準備
		HRESULT hr = commandAllocator->Reset();
		assert(SUCCEEDED(hr));
		hr = commandList->Reset(commandAllocator, nullptr);
		assert(SUCCEEDED(hr));

		// 中間リソースを解放する
		textureStore_->ReleaseIntermediateResources();
	}

	// リサイズ処理
	if (winApp_->IsResized())
		Resize(winApp_->GetClientWidth(), winApp_->GetClientHeight());

	// ビューポート、シザー矩形の設定
	commandList->RSSetViewports(1, &viewport_);
	commandList->RSSetScissorRects(1, &scissorRect_);

	// 描画用のディスクリプタヒープを設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { heap_->GetSrvDescriptorHeap() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	// デプスステンシルのクリア
	multiPass_->Clear(commandList, frameIndex_);

	// レンダーパスを実行する
	multiPass_->Execute(commandList, frameIndex_);

	// バックバッファのインデックスを取得
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBufferResource = swapChain_->GetSwapChainResource(backBufferIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferCPUHandle = swapChain_->GetSwapChainRtvHandle(backBufferIndex);

	// バックバッファリソース Present -> RenderTarget
	TransitionBarrier(backBufferResource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);

	// 描画先のRTVを設定する
	commandList->OMSetRenderTargets(1, &backBufferCPUHandle, false, nullptr);

	// 指定した色で画面全体をクリアする
	float clearColor[] = { 0.1f , 0.1f , 0.1f , 1.0f };
	commandList->ClearRenderTargetView(backBufferCPUHandle, clearColor, 0, nullptr);

	// スワップチェインにオフスクリーンリソースを書き込む
	multiPass_->RenderSwapChain(commandList, frameIndex_);

#ifdef DEVELOPMENT

	// ImGuiDockingに最終的なオフスクリーンを描画する
	imguiRender_->DrawImGuiScreen(multiPass_->GetCurrentResource(), commandList, frameIndex_);

#endif

	// バックバッファリソース RenderTarget -> Present
	TransitionBarrier(backBufferResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, commandList);

	// フレーム終了時の処理
	multiPass_->EndFrame(commandList, frameIndex_);

	// コマンドの内容を確定させる（閉じる）
	HRESULT hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// GPUにコマンドリストの実行を行わせる
	ID3D12CommandList* commandLists[] = { commandList };
	command_->GetCommandQueue()->ExecuteCommandLists(1, commandLists);

	// GPUにシグナルを送る
	fence_->SendSignal(command_->GetCommandQueue(), frameIndex_);

	// GPUとOSに画面の交換を行うよう通知する
	swapChain_->Present(0, 0);

	// 次のフレーム用のコマンドリストを準備する
	frameIndex_ = swapChain_->GetCurrentBackBufferIndex();


	// 初回フレームのフラグを下ろす
	isFirstFrame_ = false;
}

/// @brief サイズを作り直す
/// @param width 
/// @param height 
void Detail::RenderContext::Resize(int32_t width, int32_t height)
{
	if (width == 0 || height == 0) return;

	// GPUの処理が完了するまで待機する
	fence_->SendSignal(command_->GetCommandQueue(), frameIndex_);
	fence_->WaitGPU(frameIndex_);

	// コマンドリストを取得
	auto commandList = command_->GetCommandList();

	// スワップチェーンのリサイズ
	swapChain_->Resize(core_->GetDevice(), width, height);

	// オフスクリーン再生成
	multiPass_->Resize(core_->GetDevice(), commandList, width, height);

#ifdef DEVELOPMENT
	// IMGUIのリサイズ
	imguiRender_->Resize(width, height);
#endif

	// ビューポートの設定
	viewport_.Width = static_cast<float>(width);
	viewport_.Height = static_cast<float>(height);
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	// シザー矩形の設定
	scissorRect_.left = 0;
	scissorRect_.right = width;
	scissorRect_.top = 0;
	scissorRect_.bottom = height;
}