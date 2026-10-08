#pragma once
#include <memory>

#include "Logger/Logger.h"
#include "WinApp/WinApp.h"
#include "DXDebug/DXDebug.h"
#include "ECS/RegistryECS/RegistryECS.h"
#include "RenderContext/RenderContext.h"

class Engine
{
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

	/// @brief 最大バッファ数を取得する
	/// @return 
	uint32_t GetMaxBufferCount() const { return renderContext_->GetMaxBufferCount(); }

	/// @brief フレームインデックスを取得する
	/// @return 
	uint32_t GetFrameIndex() const { return renderContext_->GetFrameIndex(); }

	/// @brief シーン前処理
	void PerScene();

	/// @brief 新フレーム処理
	void NewFrame();

	/// @brief 描画後処理
	void PostDraw();

	/// @brief ログ出力
	/// @brief level
	/// @param log 
	void Log(LogLevel level, const std::string& log) { logger_->Logging(level, log); }

	/// @brief ECSレジストリを取得する
	/// @return 
	Detail::RegistryECS* GetRegistryECS() { return registry_.get(); }

	/// @brief レンダーパスを作成する
	/// @param priority 
	/// @param blendMode 
	/// @param drawFunc 
	/// @return 
	Entity CreatePass(int priority, BlendMode blendMode, std::function<void()> drawFunc) { return renderContext_->CreatePass(priority, blendMode, drawFunc); }

	/// @brief テクスチャを読み込む
	/// @param filePath 
	/// @return 
	uint32_t LoadTexture(const std::string& filePath) { return renderContext_->LoadTexture(filePath); }

	/// @brief PSOエディタを取得する
	/// @return 
	Detail::PSOEditor* GetPSOEditor() { return renderContext_->GetPSOEditor(); }

	/// @brief シェーダエディタを取得する
	/// @return 
	Detail::ShaderEditor* GetShaderEditor() { return renderContext_->GetShaderEditor(); }

	/// @brief テクスチャファイルがドロップされたときの処理
	/// @param filePath 
	void OnTextureDropped(const std::string& filePath) { renderContext_->OnTextureDropped(filePath); }



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

	/// @brief ウィンドウアプリケーション
	std::unique_ptr<Detail::WinApp> winApp_;

	/// @brief ECSレジストリ
	std::unique_ptr<Detail::RegistryECS> registry_;

#ifdef _DEBUG

	/// @brief DirectXデバッグ
	std::unique_ptr<Detail::DXDebug> dxDebug_;

#endif

	/// @brief レンダーコンテキスト
	std::unique_ptr<Detail::RenderContext> renderContext_;
};

