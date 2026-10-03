#include "RenderContext.h"
#include "WinApp/WinApp.h"

/// @brief コンストラクタ
Detail::RenderContext::RenderContext(WinApp* winApp)
{
	// DXCoreを作成
	core_ = std::make_unique<DXCore>();

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
}

/// @brief シーン前処理
void Detail::RenderContext::NewFrame()
{

}

/// @brief 描画前処理
void Detail::RenderContext::PreDraw()
{

}

/// @brief 描画後処理
void Detail::RenderContext::PostDraw()
{

}