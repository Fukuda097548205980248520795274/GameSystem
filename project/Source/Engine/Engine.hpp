#pragma once
#include <memory>
#include <angelscript.h>

#include "Logger/Logger.hpp"
#include "WinApp/WinApp.hpp"
#include "DXDebug/DXDebug.hpp"
#include "Script/ScriptSystem/ScriptSystem.hpp"
#include "ECS/RegistryECS/RegistryECS.hpp"
#include "ECS/ECSSerializer/ECSSerializer.hpp"
#include "RenderContext/RenderContext.hpp"
#include "Audio/AudioSystem/AudioSystem.hpp"
#include "Input/Input.hpp"

// ゲームの状態
enum class PlayState
{
	// 編集中
	Editor,

	// ゲームプレイ中
	Playing,

	// 一時停止
	Paused
};

/// @brief エンジン
class Engine
{
public:

	/// @brief パスキー
	struct PassKey
	{
		PassKey() = default;
		friend Engine;
	};

	/// @brief パスキーを受け取るコンストラクタ
	/// @param  
	explicit Engine(PassKey) {}


public:

	/// @brief デストラクタ
	~Engine();

	/// @brief インスタンスを取得する
	/// @param screenWidth 
	/// @param screenHeight 
	/// @param title 
	/// @return 
	static Engine* GetInstance(uint32_t screenWidth, uint32_t screenHeight, const std::string& title);

	/// @brief インスタンスを取得する
	/// @return 
	static Engine* GetInstance();

	/// @brief ゲームループ
	/// @return 
	bool GameLoop() { return winApp_->ProcessMessage(); }

	/// @brief クライアント領域の横幅を取得する
	/// @return 
	int32_t GetClientWidth() const { return winApp_->GetClientWidth(); }

	/// @brief クライアント領域の縦幅を取得する
	/// @return 
	int32_t GetClientHeight() const { return winApp_->GetClientHeight(); }

	/// @brief 最大バッファ数を取得する
	/// @return 
	uint32_t GetMaxBufferCount() const { return renderContext_->GetMaxBufferCount(); }

	/// @brief フレームインデックスを取得する
	/// @return 
	uint32_t GetFrameIndex() const { return renderContext_->GetFrameIndex(); }

	/// @brief 新フレーム処理
	void NewFrame();

	/// @brief 描画後処理
	void PostDraw();

	/// @brief GPUの処理が完了するまで待機する
	void WaitForGPU() { renderContext_->WaitForGPU(); }

	/// @brief ログ出力
	/// @brief level
	/// @param log 
	void Log(LogLevel level, const std::string& log) { logger_->Logging(level, log); }

	/// @brief ウィンドウを閉じる
	void CloseWindow() { winApp_->Close(); }

	/// @brief ゲームを停止する
	void StopExecution();

	/// @brief ECSレジストリを取得する
	/// @return 
	Detail::RegistryECS* GetRegistryECS() { return registry_.get(); }

	/// @brief Entity
	/// @return 
	Entity CreateEntity() { return registry_->CreateEntity(); }

	/// @brief レンダーパスを作成する
	/// @param priority 
	/// @param blendMode 
	/// @param drawFunc 
	/// @return 
	Entity CreatePass(int priority, BlendMode blendMode, std::function<void()> drawFunc) { return renderContext_->CreatePass(priority, blendMode, drawFunc); }

	/// @brief レンダー3Dエンティティを作成する
	/// @return 
	Entity CreateRender3DEntity() { return renderContext_->CreateRender3DEntity(); }

	/// @brief モデルエンティティを作成する
	/// @param modelHandle 
	/// @return 
	Entity CreateModelEntity(Detail::ModelHandle modelHandle) { return renderContext_->CreateModelEntity(modelHandle); }

	/// @brief カメラエンティティを作成する
	/// @return 
	Entity CreateCameraEntity() { return renderContext_->CreateCameraEntity(); }

	/// @brief Skyboxエンティティを作成する
	/// @return 
	Entity CreateSkyboxEntity() { return renderContext_->CreateSkyboxEntity(); }

	/// @brief オーディオエンティティを作成する
	/// @param audioHandle 
	/// @param volume 
	/// @param pitch 
	/// @param isLoop 
	/// @return 
	Entity CreateAudioEntity(Detail::AudioHandle audioHandle, float volume = 0.5f, float pitch = 1.0f, bool isLoop = false) { return audioSystem_->CreateAudioEntity(audioHandle, volume, pitch, isLoop); }

	/// @brief 平行光源エンティティを作成する
	/// @param direction 
	/// @param color 
	/// @param intensity 
	/// @return 
	Entity CreateDirectionalLightEntity(const Vector3& direction, const Vector4& color, float intensity) { return renderContext_->CreateDirectionalLightEntity(direction, color, intensity); }

	/// @brief ポイントライトエンティティを作成する
	/// @param position 
	/// @param color 
	/// @param intensity 
	/// @param radius 
	/// @param decay 
	/// @return 
	Entity CreatePointLightEntity(const Vector3& position, const Vector4& color, float intensity, float radius, float decay) { return renderContext_->CreatePointLightEntity(position, color, intensity, radius, decay); }

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
	Entity CreateSpotLightEntity(const Vector3& position, const Vector3& direction, const Vector4& color, float intensity, float radius, float decay, float cosAngle, float cosFalloffStart) { return renderContext_->CreateSpotLightEntity(position, direction, color, intensity, radius, decay, cosAngle, cosFalloffStart); }

	/// @brief テクスチャを読み込む
	/// @param filePath 
	/// @return 
	Detail::TextureHandle LoadTexture(const std::string& filePath) { return renderContext_->LoadTexture(filePath); }

	/// @brief モデルを読み込む
	/// @param filePath 
	/// @return 
	Detail::ModelHandle LoadModel(const std::string& filePath) { return renderContext_->LoadModel(filePath); }

	/// @brief レンダーコンテキストを取得する
	/// @return 
	Detail::RenderContext* GetRenderContext() { return renderContext_.get(); }

	/// @brief PSOエディタを取得する
	/// @return 
	Detail::PSOEditor* GetPSOEditor() { return renderContext_->GetPSOEditor(); }

	/// @brief シェーダエディタを取得する
	/// @return 
	Detail::ShaderEditor* GetShaderEditor() { return renderContext_->GetShaderEditor(); }

	/// @brief 入力処理を取得する
	/// @return 
	Detail::Input* GetInput() { return input_.get(); }

	/// @brief オーディオシステムを取得する
	/// @return 
	Detail::AudioSystem* GetAudioSystem() { return audioSystem_.get(); }

	/// @brief スクリプトシステムを取得する
	/// @return 
	Detail::ScriptSystem* GetScriptSystem() { return scriptSystem_.get(); }

	/// @brief テクスチャファイルがドロップされたときの処理
	/// @param filePath 
	void OnTextureDropped(const std::string& filePath) { renderContext_->OnTextureDropped(filePath); }

	/// @brief オーディオを読み込む
	/// @param filePath 
	/// @return 
	Detail::AudioHandle LoadAudio(const std::string& filePath) { return audioSystem_->Load(filePath); }


	/// @brief プレイ状態を取得
	/// @return 
	PlayState GetPlayState() const { return playState_; }

	/// @brief プレイ状態を設定
	/// @param state 
	void SetPlayState(PlayState state) { playState_ = state; }

	/// @brief ECSレジストリを設定する
	/// @param registry 
	void SetRegistryECS(std::unique_ptr<Detail::RegistryECS> registry) { registry_ = std::move(registry); }

	/// @brief ECSレジストリの復元を予約する
	/// @param registry 
	void QueueRestoreRegistry(std::unique_ptr<Detail::RegistryECS> registry) { pendingRestoreRegistry_ = std::move(registry); }



	/// @brief 現在のECS状態をファイル（JSON / BIN）に保存する
	/// @param sceneName 
	/// @return 
	bool SaveScene(const std::string& sceneName = "Scene") { return Detail::ToJson(registry_.get(), sceneName); }

	/// @brief JSONからECS状態を読み込む
	/// @param sceneName 
	/// @return 
	bool LoadSceneFromJSON(const std::string& sceneName = "Scene") { return Detail::LoadJson(registry_.get(), sceneName); }

	/// @brief バイナリからECS状態を読み込む（Cookedデータ用）
	/// @param sceneName 
	/// @return 
	bool LoadSceneFromBinary(const std::string& sceneName = "Scene") { return Detail::LoadBin(registry_.get(), sceneName); }

	/// @brief スクリプトエンジンを登録する
	/// @param moduleName 
	/// @param scriptPath 
	/// @return 
	Detail::ScriptHandle BuildScriptModule(const std::string& moduleName, const std::string& scriptPath) { return scriptSystem_->BuildScriptModule(moduleName, scriptPath); }



private:

	/// @brief インスタンス
	static std::unique_ptr<Engine> instance_;

	/// @brief コンストラクタ
	Engine() = default;
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;


private:

	/// @brief 初期化
	/// @param screenWidth 
	/// @param screenHeight 
	/// @param title 
	void Initialize(int32_t screenWidth, int32_t screenHeight, const std::string& title);

	/// @brief ロガー
	std::unique_ptr<Detail::Logger> logger_;

	/// @brief スクリプトシステム
	std::unique_ptr<Detail::ScriptSystem> scriptSystem_;

	/// @brief ウィンドウアプリケーション
	std::unique_ptr<Detail::WinApp> winApp_;

	/// @brief ECSレジストリ
	std::unique_ptr<Detail::RegistryECS> registry_;

	/// @brief 入力処理
	std::unique_ptr<Detail::Input> input_;

	/// @brief オーディオシステム
	std::unique_ptr<Detail::AudioSystem> audioSystem_;


#ifdef _DEBUG

	/// @brief DirectXデバッグ
	std::unique_ptr<Detail::DXDebug> dxDebug_;

#endif

	/// @brief レンダーコンテキスト
	std::unique_ptr<Detail::RenderContext> renderContext_;



	// ゲームの状態
	PlayState playState_ = PlayState::Playing;

	// 次のフレームで差し替えるための予約用レジストリ
	std::unique_ptr<Detail::RegistryECS> pendingRestoreRegistry_ = nullptr;
};

