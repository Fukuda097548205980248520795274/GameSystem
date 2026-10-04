#include "RenderTargetPool.h"
#include "Engine.h"
#include "Func/ConvertString/ConvertString.h"

/// @brief 初期化
/// @param device 
/// @param heap 
/// @param buffering 
/// @param poolSize 
/// @param commandList 
void Detail::RenderTargetPool::Initialize(ID3D12Device* device, DXHeap* heap, DXSwapChain* swapChain, ID3D12GraphicsCommandList* commandList, int poolSize)
{
	// nullptrチェック
	assert(device);
	assert(heap);
	assert(swapChain);
	assert(commandList);

	// 引数を受け取る
	device_ = device;
	heap_ = heap;
	swapChain_ = swapChain;

	int width = static_cast<int>(swapChain_->GetSwapChainDesc().Width);
	int height = static_cast<int>(swapChain_->GetSwapChainDesc().Height);

	for (int i = 0; i < poolSize; ++i)
	{
		CreateRenderTarget(width, height, commandList);
	}
}

/// @brief 空いているレンダーターゲットを借りる
/// @return 
Detail::OffscreenResource* Detail::RenderTargetPool::Rent(ID3D12GraphicsCommandList* commandList)
{
	// プールが空の場合は新しいレンダーターゲットを作成
	if (freeQueue_.empty())
	{
		int width = static_cast<int>(swapChain_->GetSwapChainDesc().Width);
		int height = static_cast<int>(swapChain_->GetSwapChainDesc().Height);

		CreateRenderTarget(width, height, commandList);
	}

	// プールからレンダーターゲットを取得
	OffscreenResource* renderTarget = freeQueue_.front();
	freeQueue_.pop();

	return renderTarget;
}

/// @brief 使い終わったレンダーターゲットを返却する
/// @param renderTarget 
void Detail::RenderTargetPool::Return(OffscreenResource* resource)
{
	// nullptrチェック
	if (!resource)return;

	// プールに返却
	freeQueue_.push(resource);
}

/// @brief フレーム終了時にリソースがすべて返却されているか確認する
void Detail::RenderTargetPool::CheckMemoryLeaks()
{
	// フレームの最後で、貸し出したリソースが全て返ってきているか確認
	assert(freeQueue_.size() == resources_.size() && "返却されていないオフスクリーンリソースがある");
}

/// @brief サイズを作り直す
/// @param device 
/// @param width 
/// @param height 
void Detail::RenderTargetPool::Resize(int width, int height, ID3D12GraphicsCommandList* commandList)
{
	auto engine = Engine::GetInstance();
	int frameIndex = engine->GetFrameIndex();

	for (auto& resource : resources_)
	{
		resource->Resize(device_, width, height);

		// バリアを張る
		resource->AllBarrier(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}

/// @brief レンダーターゲットを作成する
/// @param width 
/// @param height 
void Detail::RenderTargetPool::CreateRenderTarget(int width, int height, ID3D12GraphicsCommandList* commandList)
{
	auto engine = Engine::GetInstance();
	int frameIndex = engine->GetFrameIndex();

	// レンダーターゲットを作成
	auto renderTarget = std::make_unique<OffscreenResource>(device_, heap_, width, height);
	std::string name = "RenderTarget_" + std::to_string(resources_.size());
	renderTarget->SetName(name);
	renderTarget->AllBarrier(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// プールに追加
	freeQueue_.push(renderTarget.get());
	resources_.push_back(std::move(renderTarget));
}