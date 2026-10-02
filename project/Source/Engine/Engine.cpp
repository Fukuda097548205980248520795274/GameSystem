#include "Engine.h"
#include "Func/CrashHandler/CrashHandler.h"

#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"Dbghelp.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "Mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "xaudio2.lib")

// インスタンス
std::unique_ptr<Engine> Engine::instance_ = nullptr;

/// @brief デストラクタ
Engine::~Engine()
{
	// レンダーコンテキストを破棄
	renderContext_.reset();
	renderContext_ = nullptr;

#ifdef _DEBUG
	// DirectXデバッグを破棄
	dxDebug_.reset();
	dxDebug_ = nullptr;
#endif

	// ウィンドウアプリケーションを破棄
	winApp_.reset();
	winApp_ = nullptr;

	// ロガーを破棄
	logger_.reset();
	logger_ = nullptr;

	// COM終了
	CoUninitialize();
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
	// 例外が発生したときに起動する
	Detail::InitializeCrashHandler();

	// COM初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// ロガーを作成
	logger_ = std::make_unique<Detail::Logger>();

	// ウィンドウアプリケーションを作成
	winApp_ = std::make_unique<Detail::WinApp>();
	winApp_->Initialize(screenWidth, screenHeight, title);

#ifdef _DEBUG
	// DirectXデバッグを作成
	dxDebug_ = std::make_unique<Detail::DXDebug>();
#endif

	// レンダーコンテキストを作成
	renderContext_ = std::make_unique<Detail::RenderContext>();
}