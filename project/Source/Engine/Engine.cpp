#include "Engine.h"

// インスタンス
std::unique_ptr<Engine> Engine::instance_ = nullptr;

/// @brief デストラクタ
Engine::~Engine()
{
	// ウィンドウアプリケーションを破棄
	winApp_.reset();
	winApp_ = nullptr;
}

/// @brief インスタンスを取得する
/// @param screenWidth 
/// @param screenHeight 
/// @param title 
/// @return 
Engine* Engine::GetInstance(uint32_t screenWidth, uint32_t screenHeight, const std::string& title)
{
	if (!instance_)
	{
		instance_.reset(new Engine());
		instance_->Initialize(screenWidth, screenHeight, title);
	}

	return instance_.get();
}

/// @brief インスタンスを取得する
/// @return 
Engine* Engine::GetInstance()
{
	if (!instance_)
		return nullptr;

	return instance_.get();
}

/// @brief シーン前処理
void Engine::PerScene()
{

}

/// @brief 新フレーム処理
void Engine::NewFrame()
{

}

/// @brief 描画前処理
void Engine::PreDraw()
{

}

/// @brief 描画後処理
void Engine::PostDraw()
{

}

/// @brief 初期化
/// @param screenWidth 
/// @param screenHeight 
/// @param title 
void Engine::Initialize(int32_t screenWidth, int32_t screenHeight, const std::string& title)
{
	// ウィンドウアプリケーションを作成
	winApp_ = std::make_unique<Detail::WinApp>();
	winApp_->Initialize(screenWidth, screenHeight, title);
}