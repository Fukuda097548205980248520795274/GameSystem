#pragma once
#include <Windows.h>
#include <string>
#include <cstdint>
#include <functional>

namespace Detail
{
	class WinApp
	{
	public:

		/// @brief ウィンドウプロシージャ
		/// @param hwnd 
		/// @param msg 
		/// @param wparam 
		/// @param lparam 
		/// @return 
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


	public:

		/// @brief デストラクタ
		~WinApp();

		/// @brief 初期化
		/// @param clientWidth 
		/// @param clientHeight 
		/// @param title 
		void Initialize(int32_t clientWidth, int32_t clientHeight, const std::string& title);

		/// @brief 更新処理
		void Update();

		/// @brief ウィンドウにメッセージを渡して応答する
		/// @return 
		bool ProcessMessage();

		/// @brief クライアント領域横幅を取得する
		/// @return 
		int32_t GetClientWidth()const { return clientWidth_; }

		/// @brief クライアント領域縦幅を取得する
		/// @return 
		int32_t GetClientHeight()const { return clientHeight_; }

		/// @brief ウィンドウクラスを取得する
		/// @return 
		const WNDCLASS& GetWndClass()const { return wc_; }

		/// @brief ウィンドウハンドルを取得する
		/// @return 
		const HWND& GetHwnd()const { return  hwnd_; }

		/// @brief リサイズしたかどうか
		/// @return 
		bool IsResized()const { return isResized_; }

		/// @brief フルスクリーントグル
		void Fullscreen();

		/// @brief 閉じる
		void Close() { PostQuitMessage(0); }

		/// @brief ドロップされたファイルを処理するコールバック関数を設定する
		/// @param callback 
		void SetDropCallback(std::function<void(const std::string&)> callback) { dropCallback_ = callback; }


	private:

		// ウィンドウクラス
		WNDCLASS wc_{};

		/// @brief ウィンドウ矩形
		RECT wrc_{};

		// ウィンドウハンドル
		HWND hwnd_;


	private:

		// クライアント領域の横幅
		int32_t clientWidth_ = 0;

		// クライアント領域の縦幅
		int32_t clientHeight_ = 0;

		// 直前のクライアント領域の横幅
		int32_t prevClientWidth_ = 0;

		// 直前のクライアント領域の縦幅
		int32_t prevClientHeight_ = 0;


		/// @brief リサイズしたかどうか
		bool isResized_ = false;


	private:

		/// @brief フルスクリーン
		bool isFullscreen_ = false;

		/// @brief ウィンドウの位置とサイズを保存する構造体
		WINDOWPLACEMENT windowPlacement_{ sizeof(WINDOWPLACEMENT) };

		/// @brief ウィンドウスタイル
		DWORD windowStyle_ = 0;


	private:

		/// @brief ドロップされたファイルを処理するコールバック関数
		std::function<void(const std::string&)> dropCallback_;
	};
}