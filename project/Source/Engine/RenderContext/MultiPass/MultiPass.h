#pragma once
#include "RenderTargetPool/RenderTargetPool.h"
#include "SwapChainCopyPSO/SwapChainCopyPSO.h"
#include "RenderPassSystem/RenderPassSystem.h"
#include "Resource/DepthResource/DepthResource.h"

namespace Detail
{
	class ShaderCompiler;
	class RenderSystem;

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

		/// @brief レンダーパスを作成する
		/// @param priority 
		/// @param blendMode 
		/// @param drawFunc 
		/// @return 
		Entity CreatePass(int priority, BlendMode blendMode, std::function<void()> drawFunc) { return renderPassSystem_->CreatePass(priority, blendMode, drawFunc); }

		/// @brief クリア
		/// @param commandList 
		void Clear(ID3D12GraphicsCommandList* commandList, int frameIndex);

		/// @brief フレーム終了時の処理
		/// @param commandList 
		void EndFrame(ID3D12GraphicsCommandList* commandList, int frameIndex);

		/// @brief スワップチェインのRTVリソースにオフクリーンリソースを書き込む
		/// @param commandList 
		/// @param frameIndex
		void RenderSwapChain(ID3D12GraphicsCommandList* commandList, int frameIndex);

		/// @brief レンダーパスを実行する
		/// @param commandList 
		void Execute(ID3D12GraphicsCommandList* commandList, RenderSystem* renderSystem, int frameIndex);

		/// @brief 現在のレンダーパスのリソースを設定する
		/// @param resource 
		void SetCurrentResource(OffscreenResource* resource) { currentResource_ = resource; }

		/// @brief 現在のレンダーターゲットのリソースを取得する
		/// @return 
		OffscreenResource* GetCurrentResource() const { return currentResource_; }

		/// @brief サイズを作り直す
		/// @param device 
		/// @param commandList 
		/// @param width 
		/// @param height 
		void Resize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, int width, int height);


	private:

		/// @brief 初期化
		/// @param device 
		/// @param heap 
		/// @param swapChain 
		/// @param commandList 
		/// @param shaderCompiler 
		void Initialize(ID3D12Device* device, DXHeap* heap, DXSwapChain* swapChain, ID3D12GraphicsCommandList* commandList, ShaderCompiler* shaderCompiler);


	private:

		// 読み込みのリソース
		OffscreenResource* sourceResource_ = nullptr;

		// 書き込み対象のレンダーターゲット
		OffscreenResource* destinationResource_ = nullptr;

		// 最新のパスの結果
		OffscreenResource* currentResource_ = nullptr;


	private:

		/// @brief 深度リソース
		std::unique_ptr<DepthResource> depthResource_;

		/// @brief レンダーターゲットプール
		std::unique_ptr<RenderTargetPool> renderTargetPool_;

		/// @brief スワップチェーンコピーPSO
		std::unique_ptr<SwapChainCopyPSO> swapChainCopyPSO_;

		/// @brief レンダーパスシステム
		std::unique_ptr< RenderPassSystem> renderPassSystem_;


	private:

		/// @brief ヒープ
		DXHeap* heap_;

		/// @brief DXSwapChain
		DXSwapChain* buffering_;
	};
}