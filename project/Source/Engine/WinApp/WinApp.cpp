#include "WinApp.h"
#include <format>
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include "Func/ConvertString/ConvertString.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/// @brief ウィンドウプロシージャ
/// @param hwnd 
/// @param msg 
/// @param wparam 
/// @param lparam 
/// @return 
LRESULT CALLBACK Detail::WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ImGuiを操作すると途中で打ち切ることができる
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}

	// メッセージに応じて固有の処理を行う
	switch (msg)
	{
	case WM_DESTROY:
		// ウィンドウが破棄された

		// OSに対してアプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}



/// @brief デストラクタ
Detail::WinApp::~WinApp()
{
	// OSの精度を戻す
	timeEndPeriod(1);

	// ウィンドウハンドルを破棄する
	if (hwnd_)
		DestroyWindow(hwnd_);
}


/// @brief 初期化
/// @param clientWidth 
/// @param clientHeight 
/// @param title 
void Detail::WinApp::Initialize(int32_t clientWidth, int32_t clientHeight, const std::string& title)
{

	// 引数を受け取る
	clientWidth_ = clientWidth;
	clientHeight_ = clientHeight;

	// システムタイマーの分解能を上げる
	timeBeginPeriod(1);


	/*---------------------------
		ウィンドウクラスを登録する
	---------------------------*/

	// ウィンドウプロシージャ
	wc_.lpfnWndProc = WindowProc;

	// ウィンドウクラス名
	wc_.lpszClassName = L"Growth";

	// インスタンスハンドル
	wc_.hInstance = GetModuleHandle(nullptr);

	// カーソル
	wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// 登録する
	RegisterClass(&wc_);


	/*---------------------------
		ウィンドウサイズを決める
	---------------------------*/

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	wrc_ = { 0 , 0 , clientWidth_ , clientHeight_ };

	// クライアント領域をもとに、実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc_, WS_OVERLAPPEDWINDOW, false);


	/*---------------------------
		ウィンドウを生成して表示
	---------------------------*/

	// ウィンドウの生成
	hwnd_ = CreateWindow(
		// 利用するクラス名
		wc_.lpszClassName,

		// タイトルバーの文字
		ConvertString(title).c_str(),

		// ウィンドウスタイル
		WS_OVERLAPPEDWINDOW,

		// 表示座標
		CW_USEDEFAULT,
		CW_USEDEFAULT,

		// ウィンドウの大きさ
		wrc_.right - wrc_.left,
		wrc_.bottom - wrc_.top,

		// 親ウィンドウハンドル
		nullptr,

		// メニューハンドル
		nullptr,

		// インスタンスハンドル
		wc_.hInstance,

		// オプション
		nullptr
	);

	// ウィンドウを表示する
	ShowWindow(hwnd_, SW_SHOW);
}

/// @brief 更新処理
void Detail::WinApp::Update()
{
	prevClientWidth_ = clientWidth_;
	prevClientHeight_ = clientHeight_;

	GetClientRect(hwnd_, &wrc_);

	clientWidth_ = wrc_.right - wrc_.left;
	clientHeight_ = wrc_.bottom - wrc_.top;

	isResized_ = (clientWidth_ != prevClientWidth_) || (clientHeight_ != prevClientHeight_);
}

/// @brief ウィンドウにメッセージを渡して応答する
/// @return 
bool Detail::WinApp::ProcessMessage()
{
	MSG msg{};

	// ウィンドウにメッセージが来ていたら最優先で処理させる
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		// ウィンドウでxボタンが押されたら終了
		if (msg.message == WM_QUIT)return false;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

/// @brief フルスクリーントグル
void Detail::WinApp::Fullscreen()
{
	isFullscreen_ = !isFullscreen_;

	if (isFullscreen_)
	{
		// 現在のウィンドウ情報保存
		windowStyle_ = GetWindowLong(hwnd_, GWL_STYLE);
		GetWindowPlacement(hwnd_, &windowPlacement_);

		MONITORINFO mi{ sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY), &mi);

		// 枠を外す
		SetWindowLong(hwnd_, GWL_STYLE, windowStyle_ & ~WS_OVERLAPPEDWINDOW);

		// モニタサイズにする
		SetWindowPos(hwnd_, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	} 
	else
	{
		// 元に戻す
		SetWindowLong(hwnd_, GWL_STYLE, windowStyle_);

		SetWindowPlacement(hwnd_, &windowPlacement_);

		SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}