#pragma once
#include <memory>

#include "Logger/Logger.h"
#include "WinApp/WinApp.h"

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

	/// @brief シーン前処理
	void PerScene();

	/// @brief 新フレーム処理
	void NewFrame();

	/// @brief 描画前処理
	void PreDraw();

	/// @brief 描画後処理
	void PostDraw();

	/// @brief ログ出力
	/// @brief level
	/// @param log 
	void Log(LogLevel level, const std::string& log) { logger_->Logging(level, log); }



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

};

