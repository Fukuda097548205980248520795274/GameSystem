#pragma once
#include "DXCore/DXCore.hpp"
#include "DXCommand/DXCommand.hpp"
#include "DXFence/DXFence.hpp"
#include "DXHeap/DXHeap.hpp"
#include "DXSwapChain/DXSwapChain.hpp"
#include "Shader/ShaderCompiler/ShaderCompiler.hpp"
#include "MultiPass/MultiPass.hpp"
#include "Store/TextureStore/TextureStore.hpp"
#include "Store/ModelStore/ModelStore.hpp"
#include "Store/ProceduralMeshStore/ProceduralMeshStore.hpp"
#include "ImGuiRender/ImGuiRender.hpp"

#include "Render/CameraSystem/CameraSystem.hpp"
#include "Render/RenderSystem/RenderSystem.hpp"
#include "Render/LightSystem/LightSystem.hpp"
#include "Render/SkyboxSystem/SkyboxSystem.hpp"

#include "ProfilerEditor/ProfilerEditor.hpp"
#include "ECS/ECSEditor/ECSEditor.hpp"
#include "Shader/ShaderEditor/ShaderEditor.hpp"
#include "PSO/PSOEditor/PSOEditor.hpp"
#include "Texture/TextureEditor/TextureEditor.hpp"
#include "Model/ModelEditor/ModelEditor.hpp"

#include <memory>

namespace Detail
{
	/// @brief ウィンドウアプリケーション
	class WinApp;

	/// @brief DXデバッグ
	class DXDebug;

	/// @brief レンダーコンテキスト
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

		/// @brief モデルエンティティを作成する
		/// @param modelHandle 
		/// @return 
		Entity CreateModelEntity(ModelHandle modelHandle) { return renderSystem_->CreateModelEntity(modelHandle); }

		/// @brief カメラエンティティを作成する
		/// @return 
		Entity CreateCameraEntity() { return cameraSystem_->CreateCameraEntity(); }

		/// @brief Skyboxエンティティを作成する
		/// @return 
		Entity CreateSkyboxEntity() { return skyboxSystem_->CreateSkyboxEntity(); }

		/// @brief 平行光源エンティティを作成する
		/// @param direction 
		/// @param color 
		/// @param intensity 
		/// @return 
		Entity CreateDirectionalLightEntity(const Vector3& direction, const Vector4& color, float intensity) { return lightSystem_->CreateDirectionalLightEntity(direction, color, intensity); }

		/// @brief ポイントライトエンティティを作成する
		/// @param position 
		/// @param color 
		/// @param intensity 
		/// @param radius 
		/// @param decay 
		/// @return 
		Entity CreatePointLightEntity(const Vector3& position, const Vector4& color, float intensity, float radius, float decay) { return lightSystem_->CreatePointLightEntity(position, color, intensity, radius, decay); }

		/// @brief スポットライトエンティティを作成する
		/// @param position 
		/// @param direction 
		/// @param color 
		/// @param intensity 
		/// @param radius 
		/// @param decay 
		/// @param cosAngle 
		/// @param cosFalloffStart 
		/// @return 
		Entity CreateSpotLightEntity(const Vector3& position, const Vector3& direction, const Vector4& color, float intensity, float radius, float decay, float cosAngle, float cosFalloffStart) { return lightSystem_->CreateSpotLightEntity(position, direction, color, intensity, radius, decay, cosAngle, cosFalloffStart); }

		/// @brief テクスチャを読み込む
		/// @param filePath 
		/// @return 
		TextureHandle LoadTexture(const std::string& filePath) { return textureStore_->Load(filePath, heap_.get(), core_->GetDevice(), command_->GetCommandList()); }

		/// @brief モデルを読み込む
		/// @param dir 
		/// @param fileName 
		/// @return 
		ModelHandle LoadModel(const std::string& filePath) { return modelStore_->Load(filePath, textureStore_.get(), heap_.get(), core_->GetDevice(), command_->GetCommandList()); }

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

		/// @brief モデルストアを取得する
		/// @return 
		ModelStore* GetModelStore() const { return modelStore_.get(); }

		/// @brief プロシージャルメッシュストアを取得する
		/// @return 
		ProceduralMeshStore* GetProceduralMeshStore() const { return proceduralMeshStore_.get(); }

		/// @brief カメラシステムを取得する
		/// @return 
		CameraSystem* GetCameraSystem() const { return cameraSystem_.get(); }

		/// @brief ライトシステムを取得する
		/// @return 
		LightSystem* GetLightSystem() const { return lightSystem_.get(); }

		/// @brief スワップチェーンを取得する
		/// @return 
		DXSwapChain* GetSwapChain() const { return swapChain_.get(); }

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

		/// @brief モデルストア
		std::unique_ptr<ModelStore> modelStore_ = nullptr;

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

		/// @brief ライトシステム
		std::unique_ptr<LightSystem> lightSystem_ = nullptr;

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

		/// @brief モデルエディタ
		std::unique_ptr<ModelEditor> modelEditor_ = nullptr;
	};
}