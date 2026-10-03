#include "DXSwapChain.h"
#include <format>
#include <cassert>
#include "RenderContext/DXHeap/DXHeap.h"
#include "WinApp/WinApp.h"
#include "RenderContext/DXCore/DXCore.h"
#include "RenderContext/DXCommand/DXCommand.h"

#include "Engine.h"

/// @brief 初期化
/// @param heap 
/// @param winApp 
/// @param dxCore 
/// @param dxCommand 
void Detail::DXSwapChain::Initialize(DXHeap* heap, WinApp* winApp, DXCore* dxCore, DXCommand* dxCommand)
{
	// nullptrチェック
	assert(heap);
	assert(winApp);
	assert(dxCore);
	assert(dxCommand);

	// dxgiFactoryとコマンドキューを取得する
	auto dxgiFactory = dxCore->GetDXGIFactory();
	auto commandQueue = dxCommand->GetCommandQueue();
	auto device = dxCore->GetDevice();
	auto engine = Engine::GetInstance();


	// 最大バッファ数を取得する
	int32_t maxBufferCount = 2;
	if (engine)
	{
		maxBufferCount = engine->GetMaxBufferCount();
	}

	// スワップチェーンのリソースとRTVハンドルの配列を確保する
	swapChainResource_.resize(maxBufferCount);
	rtvCPUHandle_.resize(maxBufferCount);


	/*-----------------------------
		スワップチェーンを生成する
	-----------------------------*/

	// クライアント領域
	swapChainDesc_.Width = winApp->GetClientWidth();
	swapChainDesc_.Height = winApp->GetClientHeight();

	// 色の形式
	swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// マルチサンプルしない
	swapChainDesc_.SampleDesc.Count = 1;

	// 描画のターゲットとして利用する
	swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// ダブルバッファ
	swapChainDesc_.BufferCount = maxBufferCount;

	// モニタに移したら中身を破棄
	swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// 生成する
	HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, winApp->GetHwnd(),
		&swapChainDesc_, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	if (!SUCCEEDED(hr))
	{
		throw std::runtime_error("スワップチェインの作成に失敗しました。");
	}

	if (engine)engine->Log(LogLevel::Info, "スワップチェイン生成");



	/*-------------------
		RTVを作成する
	-------------------*/

	// 出力結果をSRGBに変換して書き込む
	rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// 2Dテクスチャとして書き込む
	rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int32_t i = 0; i < maxBufferCount; ++i)
	{
		// リソース取得
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResource_[i]));
		if (!SUCCEEDED(hr)) throw std::runtime_error("スワップチェインリソースの取得に失敗しました。");
		if (engine)engine->Log(LogLevel::Info, std::format("スワップチェインリソース取得 : {}", i));

		// RTVの作成
		rtvCPUHandle_[i] = heap->GetRtvDescriptorHandle();
		device->CreateRenderTargetView(swapChainResource_[i].Get(), &rtvDesc_, rtvCPUHandle_[i]);
		if (engine)engine->Log(LogLevel::Info, std::format("RTV作成 : {}", i));
	}
}

/// @brief サイズを作り直す
/// @param device 
/// @param width 
/// @param height 
void Detail::DXSwapChain::Resize(ID3D12Device* device, int32_t width, int32_t height)
{
	if (width <= 0 || height <= 0) return;

	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();

	// 最大バッファ数を取得する
	int32_t maxBufferCount = static_cast<int32_t>(swapChainResource_.size());

	// スワップチェイン再設定
	swapChainDesc_.Width = width;
	swapChainDesc_.Height = height;

	// スワップチェインのリソースを解放
	for (int32_t i = 0; i < maxBufferCount; ++i) 
	{
		swapChainResource_[i].Reset();
	}

	// リサイズ
	HRESULT hr = swapChain_->ResizeBuffers(maxBufferCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	if (!SUCCEEDED(hr))throw std::runtime_error("スワップチェインのリサイズに失敗しました。");
	if (engine)engine->Log(LogLevel::Info, std::format("スワップチェインリサイズ : {} x {}", width, height));


	// 再取得とRTV再生成
	for (int32_t i = 0; i < maxBufferCount; ++i)
	{
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResource_[i]));
		if (!SUCCEEDED(hr)) throw std::runtime_error("スワップチェインリソースの取得に失敗しました。");
		if(engine)engine->Log(LogLevel::Info, std::format("スワップチェインリソース取得 : {}", i));

		device->CreateRenderTargetView(swapChainResource_[i].Get(), &rtvDesc_, rtvCPUHandle_[i]);
		if (engine)engine->Log(LogLevel::Info, std::format("RTV作成 : {}", i));
	}
}