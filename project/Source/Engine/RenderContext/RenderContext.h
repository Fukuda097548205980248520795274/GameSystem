#pragma once
#include "DXCore/DXCore.h"
#include "DXCommand/DXCommand.h"
#include "DXFence/DXFence.h"
#include "DXHeap/DXHeap.h"
#include "DXSwapChain/DXSwapChain.h"
#include "Shader/ShaderCompiler/ShaderCompiler.h"
#include "MultiPass/MultiPass.h"
#include "Store/TextureStore/TextureStore.h"
#include "Store/ProceduralMeshStore/ProceduralMeshStore.h"
#include "ImGuiRender/ImGuiRender.h"

#include "Render/CameraSystem/CameraSystem.h"
#include "Render/RenderSystem/RenderSystem.h"
#include "Render/SkyboxSystem/SkyboxSystem.h"

#include "ProfilerEditor/ProfilerEditor.h"
#include "ECS/ECSEditor/ECSEditor.h"
#include "Shader/ShaderEditor/ShaderEditor.h"
#include "PSO/PSOEditor/PSOEditor.h"
#include "Texture/TextureEditor/TextureEditor.h"

#include <memory>

namespace Detail
{
	class WinApp;
	class DXDebug;

	class RenderContext
	{
	public:

		/// @brief コンストラクタ
		RenderContext() = default;

		/// @brief デストラクタ
		~RenderContext();

		/// @brief 初期化
		/// @param winApp 
		void Initialize(WinApp* winApp, DXDebug* dxDebug);

		/// @brief 最大バッファ数を取得する
		/// @return 
		uint32_t GetMaxBufferCount() const { return kBufferCount; }

		/// @brief フレームインデックスを取得する
		/// @return 
		uint32_t GetFrameIndex() const { return frameIndex_; }

		/// @brief シーン前処理
		void NewFrame();

		/// @brief 描画後処理
		void PostDraw();

		/// @brief GPUの処理が完了するまで待機する
		void WaitForGPU() { fence_->WaitGPU(frameIndex_); }

		/// @brief レンダーパスを作成する
		/// @param priority 
		/// @param blendMode 
		/// @param drawFunc 
		/// @return 
		Entity CreatePass(int priority, BlendMode blendMode, std::function<void()> drawFunc) { return multiPass_->CreatePass(priority, blendMode, drawFunc); }

		/// @brief レンダー3Dエンティティを作成する
		/// @return 
		Entity CreateRender3DEntity() { return renderSystem_->CreateRender3DEntity(); }

		/// @brief カメラエンティティを作成する
		/// @return 
		Entity CreateCameraEntity() { return cameraSystem_->CreateCameraEntity(); }

		/// @brief Skyboxエンティティを作成する
		/// @return 
		Entity CreateSkyboxEntity() { return skyboxSystem_->CreateSkyboxEntity(); }

		/// @brief テクスチャを読み込む
		/// @param filePath 
		/// @return 
		uint32_t LoadTexture(const std::string& filePath) { return textureStore_->Load(filePath, heap_.get(), core_->GetDevice(), command_->GetCommandList()); }

		/// @brief PSOエディタを取得する
		/// @return 
		PSOEditor* GetPSOEditor() { return psoEditor_.get(); }

		/// @brief シェーダエディタを取得する
		/// @return 
		ShaderEditor* GetShaderEditor() { return shaderEditor_.get(); }

		/// @brief デバイスを取得する
		/// @return 
		DXCore* GetCore() const { return core_.get(); }

		/// @brief コマンドを取得する
		/// @return 
		DXHeap* GetHeap() const { return heap_.get(); }

		/// @brief テクスチャストアを取得する
		/// @return 
		TextureStore* GetTextureStore() const { return textureStore_.get(); }

		/// @brief プロシージャルメッシュストアを取得する
		/// @return 
		ProceduralMeshStore* GetProceduralMeshStore() const { return proceduralMeshStore_.get(); }

		/// @brief カメラシステムを取得する
		/// @return 
		CameraSystem* GetCameraSystem() const { return cameraSystem_.get(); }

		/// @brief テクスチャファイルがドロップされたときの処理
		/// @param filePath 
		void OnTextureDropped(const std::string& filePath);


	private:

		/// @brief サイズを作り直す
		/// @param width 
		/// @param height 
		void Resize(int32_t width, int32_t height);


	private:

		/// @brief DXCore
		std::unique_ptr<DXCore> core_ = nullptr;

		/// @brief DXCommand
		std::unique_ptr<DXCommand> command_ = nullptr;

		/// @brief DXFence
		std::unique_ptr<DXFence> fence_ = nullptr;

		/// @brief DXHeap
		std::unique_ptr<DXHeap> heap_ = nullptr;

		/// @brief DXSwapChain
		std::unique_ptr<DXSwapChain> swapChain_ = nullptr;

		/// @brief シェーダコンパイラ
		std::unique_ptr<ShaderCompiler> shaderCompiler_ = nullptr;

		/// @brief マルチパス
		std::unique_ptr<MultiPass> multiPass_ = nullptr;

		/// @brief テクスチャストア
		std::unique_ptr<TextureStore> textureStore_ = nullptr;

		/// @brief プロシージャルメッシュストア
		std::unique_ptr<ProceduralMeshStore> proceduralMeshStore_ = nullptr;

#ifdef DEVELOPMENT
		/// @brief ImGui描画
		std::unique_ptr<ImGuiRender> imguiRender_ = nullptr;
#endif


	private:

		/// @brief ビューポート
		D3D12_VIEWPORT viewport_{};

		/// @brief シザー矩形
		D3D12_RECT scissorRect_{};


	private:

		/// @brief カメラシステム
		std::unique_ptr<CameraSystem> cameraSystem_ = nullptr;

		/// @brief レンダーシステム
		std::unique_ptr<RenderSystem> renderSystem_ = nullptr;

		/// @brief Skyboxシステム
		std::unique_ptr<SkyboxSystem> skyboxSystem_ = nullptr;


	private:

		/// @brief バッファ数
		static constexpr uint32_t kBufferCount = 2;

		/// @brief 現在のフレームインデックス
		uint32_t frameIndex_ = 0;

		/// @brief 初回フレームかどうか
		bool isFirstFrame_ = true;


	private:

		/// @brief ウィンドウアプリケーション
		WinApp* winApp_ = nullptr;



	private:

		/// @brief プロファイラエディタ
		std::unique_ptr<ProfilerEditor> profilerEditor_ = nullptr;

		/// @brief ECSエディタ
		std::unique_ptr<ECSEditor> ecsEditor_ = nullptr;

		/// @brief PSOエディタ
		std::unique_ptr<PSOEditor> psoEditor_;

		/// @brief シェーダエディタ
		std::unique_ptr<ShaderEditor> shaderEditor_;

		/// @brief テクスチャエディタ
		std::unique_ptr<TextureEditor> textureEditor_ = nullptr;
	};
}