#pragma once
#define DIRECTINPUT_VERSION 0x0800
#define MAX_CONTROLLERS 4
#include <dinput.h>
#include <Xinput.h>
#include <wrl.h>
#include <cstdint>
#include "Vector/Vector2/Vector2.hpp"
#include "Vector/Vector3/Vector3.hpp"

namespace Detail
{
	class WinApp;

	class Input
	{
	public:

		/// @brief コンストラクタ
		/// @param winApp 
		Input(const WinApp* winApp) { Initialize(winApp); }

		/// @brief デストラクタ
		~Input() = default;

		/// @brief 全ての入力情報を取得する
		void CheckInputInfo();

		/// @brief 全ての入力情報をコピーする
		void CopyInputInfo();





		/// @brief キーの入力情報（Press）
		/// @param key 
		/// @return 
		bool GetKeyPress(BYTE key);

		/// @brief キーの入力情報（Trigger）
		/// @param key 
		/// @return 
		bool GetKeyTrigger(BYTE key);

		/// @brief キーの入力情報（Release）
		/// @param key 
		/// @return 
		bool GetKeyRelease(BYTE key);

		/// @brief マウスボタンの入力情報（Press）
		/// @param mouseButtonNumber 
		/// @return 
		bool GetMousePress(uint32_t mouseButtonNumber);

		/// @brief マウスボタンの入力情報（Trigger）
		/// @param mouseButtonNumber 
		/// @return 
		bool GetMouseTrigger(uint32_t mouseButtonNumber);

		/// @brief マウスボタンの入力情報（Release）
		/// @param mouseButtonNumber 
		/// @return 
		bool GetMouseRelease(uint32_t mouseButtonNumber);

		/// @brief マウスの移動量を取得する
		/// @return 
		Vector2 GetMouseVelocity() { return Vector2{ mouseVelocity_.x , mouseVelocity_.y }; }

		/// @brief マウスホイールが上回転したかどうか
		/// @return 
		bool GetMouseWheelUp();

		/// @brief マウスホイールが下回転したかどうか
		/// @return 
		bool GetMouseWheelDown();

		/// @brief マウスホイールの移動量を取得する
		/// @return 
		float GetMouseWheelVelocity() { return mouseVelocity_.z; }

		/// @brief ゲームパッドが有効かどうか
		/// @param gamepadNumber 
		/// @return 
		bool IsGamepadEnable(DWORD gamepadNumber);

		/// @brief ゲームパッドの入力情報（Press）
		/// @param gamepadNumber 
		/// @param wButtons 
		/// @return 
		bool GetGamepadButtonPress(DWORD gamepadNumber, DWORD wButtons);

		/// @brief ゲームパッドの入力情報（Trigger）
		/// @param gamepadNumber 
		/// @param wButtons 
		/// @return 
		bool GetGamepadButtonTrigger(DWORD gamepadNumber, DWORD wButtons);

		/// @brief ゲームパッドの入力情報（Release）
		/// @param gamepadNumber 
		/// @param wButtons 
		/// @return 
		bool GetGamepadButtonRelease(DWORD gamepadNumber, DWORD wButtons);

		/// @brief ゲームパッドの左スティックの入力情報
		/// @param gamepadNumber 
		/// @return 
		Vector2 GetGamepadLeftStick(DWORD gamepadNumber);

		/// @brief ゲームパッドの右スティックの入力情報
		/// @param gamepadNumber 
		/// @return 
		Vector2 GetGamepadRightStick(DWORD gamepadNumber);

		/// @brief ゲームパッドの左トリガーの入力情報
		/// @param gamepadNumber 
		/// @return 
		float GetGamepadLeftTrigger(DWORD gamepadNumber);

		/// @brief ゲームパッドの右トリガーの入力情報
		/// @param gamepadNumber 
		/// @return 
		float GetGamepadRightTrigger(DWORD gamepadNumber);

		/// @brief ゲームパッドを振動させる
		/// @param gamepadNumber 
		/// @param leftMotorPower 
		/// @param rightMotorPower 
		void GamepadVibration(DWORD gamepadNumber, float leftMotorPower, float rightMotorPower);



		// Microsoft::WRLを省略する
		template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	private:

		/// @brief 初期化
		/// @param winApp ウィンドウアプリケーション
		/// @param log 
		void Initialize(const WinApp* winApp);


		// DirectInput
		ComPtr<IDirectInput8> directInput_ = nullptr;


	private:

		// キーボードデバイス
		ComPtr<IDirectInputDevice8> keyboard_ = nullptr;

		// キー
		BYTE keys_[256]{};

		// 前のキー
		BYTE preKeys_[256]{};


	private:

		// マウスデバイス
		ComPtr<IDirectInputDevice8> mouse_ = nullptr;

		// マウスの入力情報
		DIMOUSESTATE mouseState_;

		// マウスの移動量
		Vector3 mouseVelocity_ = { 0.0f , 0.0f , 0.0f };

		// マウスボタンが押されたかどうか
		bool isPushButton_[3] = { false };

		// 前回のマウスボタンが押されたかどうか
		bool prevIsPushButton_[3] = { false };

	
	private:

		// 入力情報
		XINPUT_STATE gamepadState_[XUSER_MAX_COUNT]{};

		// 前回の入力情報
		XINPUT_STATE prevGamepadState_[XUSER_MAX_COUNT]{};

		// 接続確認
		DWORD dwResult_[XUSER_MAX_COUNT]{};
	};
}