#include "Engine.hpp"
#include "Func/CrashHandler/CrashHandler.hpp"
#include <scriptstdstring/scriptstdstring.h>

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

	// オーディオシステムを破棄
	audioSystem_.reset();
	audioSystem_ = nullptr;

	// 入力処理を破棄
	input_.reset();
	input_ = nullptr;

	// ECSレジストリを破棄
	registry_.reset();
	registry_ = nullptr;

	// ウィンドウアプリケーションを破棄
	winApp_.reset();
	winApp_ = nullptr;

	// スクリプトシステムを破棄
	scriptSystem_.reset();
	scriptSystem_ = nullptr;

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
		instance_ = std::make_unique<Engine>(PassKey());
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

/// @brief 新フレーム処理
void Engine::NewFrame()
{
	// 全ての入力情報を取得する
	input_->CheckInputInfo();

	// オーディオシステムの更新
	audioSystem_->Update();

	// 新フレーム処理
	renderContext_->NewFrame();

	// ウィンドウアプリケーションの更新
	winApp_->Update();
}

/// @brief 描画後処理
void Engine::PostDraw()
{
	// スクリプトシステムの更新
	scriptSystem_->Update(registry_.get());

	// 描画後処理
	renderContext_->PostDraw();

	// 全ての入力情報をコピーする
	input_->CopyInputInfo();
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

	// スクリプトシステムを作成
	scriptSystem_ = std::make_unique<Detail::ScriptSystem>();

	// ウィンドウアプリケーションを作成
	winApp_ = std::make_unique<Detail::WinApp>();
	winApp_->Initialize(screenWidth, screenHeight, title);

	// ドロップされたファイルを処理するコールバック関数を設定
	winApp_->SetDropCallback([this](const std::string& filePath) 
		{
			renderContext_->OnTextureDropped(filePath); 
		}
	);


	// ECSレジストリを作成
	registry_ = std::make_unique<Detail::RegistryECS>();

	// 入力処理を作成
	input_ = std::make_unique<Detail::Input>(winApp_.get());

	// オーディオシステムを作成
	audioSystem_ = std::make_unique<Detail::AudioSystem>();

	// DirectXデバッグのポインタ
	Detail::DXDebug* dxDebug = nullptr;

#ifdef _DEBUG
	// DirectXデバッグを作成
	dxDebug_ = std::make_unique<Detail::DXDebug>();
	dxDebug = dxDebug_.get();
#endif

	// レンダーコンテキストを作成
	renderContext_ = std::make_unique<Detail::RenderContext>();
	renderContext_->Initialize(winApp_.get(), dxDebug);
}