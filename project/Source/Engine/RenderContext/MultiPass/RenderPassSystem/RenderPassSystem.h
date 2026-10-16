#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include "ECS/EntityECS/EntityECS.h"
#include "ECS/ComponentECS/ComponentECS.h"

namespace Detail
{
	/// @brief ECSレジストリ
	class RegistryECS;

	/// @brief レンダーターゲットプール
	class RenderTargetPool;

	/// @brief マルチパス
	class MultiPass;

	/// @brief オフスクリーンリソース
	class OffscreenResource;

	/// @brief レンダーシステム
	class RenderSystem;

	/// @brief スカイボックスシステム
	class SkyboxSystem;

	/// @brief レンダーパスシステム
	class RenderPassSystem
	{
	public:

		/// @brief コンストラクタ
		/// @param renderTargetPool 
		/// @param registry 
		RenderPassSystem(RenderTargetPool* renderTargetPool) { Initialize(renderTargetPool); }

		/// @brief デストラクタ
		~RenderPassSystem() = default;

		/// @brief レンダーパスを作成する
		/// @param priority 
		/// @param blendMode 
		/// @param drawFunc 
		/// @return 
		Entity CreatePass(int priority, BlendMode blendMode, std::function<void()> drawFunc);

		/// @brief すべてのレンダーパスを実行する
		/// @param commandList 
		/// @param dsvHandle 
		/// @param multiPass 
		/// @param renderSystem 
		/// @param skyboxSystem 
		void ExecuteAll(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, MultiPass* multiPass, RenderSystem* renderSystem, SkyboxSystem* skyboxSystem);

		/// @brief レンダーパスを返却する
		void Return();


	private:

		/// @brief 初期化
		/// @param renderTargetPool 
		void Initialize(RenderTargetPool* renderTargetPool);

		/// @brief ECSレジストリ
		RegistryECS* registry_ = nullptr;

		/// @brief レンダーターゲットプール
		RenderTargetPool* renderTargetPool_ = nullptr;

		/// @brief アクティブなレンダーパスのリソース
		std::vector<OffscreenResource*> activeResources_;
	};
}