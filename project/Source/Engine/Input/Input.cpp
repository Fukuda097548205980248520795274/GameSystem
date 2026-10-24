#include "Input.h"
#include "WinApp/WinApp.h"
#include <cassert>
#include <cmath>
#include <algorithm>

/// @brief 初期化
/// @param winApp 
void Detail::Input::Initialize(const WinApp* winApp)
{
	// nullptrチェック
	assert(winApp);

	// DirectInputを初期化する
	HRESULT hr = DirectInput8Create(winApp->GetWndClass().hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput_, nullptr);
	assert(SUCCEEDED(hr));


	/*--------------------
		キーボードの生成
	--------------------*/

	// キーボードデバイスの生成
	hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
	assert(SUCCEEDED(hr));

	// 入力データの形式
	hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = keyboard_->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));


	/*----------------
		マウスの生成
	----------------*/

	// マウスデバイスの生成
	hr = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
	assert(SUCCEEDED(hr));

	// 入力データの形式セット
	hr = mouse_->SetDataFormat(&c_dfDIMouse);
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = mouse_->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(hr));
}

/// @brief 全ての入力情報を取得する
void Detail::Input::CheckInputInfo()
{
	/*--------------
		キーボード
	--------------*/

	// キーボードの入力情報を取得する
	HRESULT hr = keyboard_->Acquire();
	if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
	{
		keyboard_->Acquire();
	}

	// 取得に失敗したらキー情報をゼロクリアして押しっぱなしを防ぐ
	if (FAILED(keyboard_->GetDeviceState(sizeof(keys_), keys_)))
	{
		ZeroMemory(keys_, sizeof(keys_));
	}


	/*-----------
		マウス
	-----------*/

	// マウスの入力情報を取得する
	hr = mouse_->Acquire();
	if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
	{
		mouse_->Acquire();
	}

	// 取得に失敗したらマウス情報をゼロクリア
	if (FAILED(mouse_->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState_)))
	{
		ZeroMemory(&mouseState_, sizeof(DIMOUSESTATE));
	}

	mouseVelocity_.x = static_cast<float>(mouseState_.lX);
	mouseVelocity_.y = static_cast<float>(mouseState_.lY);
	mouseVelocity_.z = static_cast<float>(mouseState_.lZ);

	// マウスのボタンが押されたかどうか
	for (uint32_t i = 0; i < 3; i++)
	{
		isPushButton_[i] = mouseState_.rgbButtons[i];
	}


	/*-----------------
		ゲームパッド
	-----------------*/

	for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
	{
		// 構造体をゼロクリア
		ZeroMemory(&gamepadState_[i], sizeof(XINPUT_STATE));

		// ゲームパッドの入力情報を取得する
		dwResult_[i] = XInputGetState(i, &gamepadState_[i]);
	}
}

/// @brief 全ての入力情報をコピーする
void Detail::Input::CopyInputInfo()
{
	memcpy(preKeys_, keys_, 256 * sizeof(BYTE));
	memcpy(prevIsPushButton_, isPushButton_, 3 * sizeof(bool));
	memcpy(prevGamepadState_, gamepadState_, sizeof(XINPUT_STATE) * XUSER_MAX_COUNT);
}


/// @brief キーの入力情報（Press）
/// @param key 
/// @return 
bool Detail::Input::GetKeyPress(BYTE key)
{
	if (keys_[key])
	{
		return true;
	}

	return false;
}

/// @brief キーの入力情報（Trigger）
/// @param key 
/// @return 
bool Detail::Input::GetKeyTrigger(BYTE key)
{
	if (!preKeys_[key] && keys_[key])
	{
		return true;
	}

	return false;
}

/// @brief キーの入力情報（Release）
/// @param key 
/// @return 
bool Detail::Input::GetKeyRelease(BYTE key)
{
	if (preKeys_[key] && !keys_[key])
	{
		return true;
	}

	return false;
}


/// @brief マウスボタンの入力情報（Press）
/// @param mouseButtonNumber 
/// @return 
bool Detail::Input::GetMousePress(uint32_t mouseButtonNumber)
{
	if (isPushButton_[mouseButtonNumber])
	{
		return true;
	}

	return false;
}

/// @brief マウスボタンの入力情報（Trigger）
/// @param mouseButtonNumber 
/// @return 
bool Detail::Input::GetMouseTrigger(uint32_t mouseButtonNumber)
{
	if (!prevIsPushButton_[mouseButtonNumber] && isPushButton_[mouseButtonNumber])
	{
		return true;
	}

	return false;
}

/// @brief マウスボタンの入力情報（Release）
/// @param mouseButtonNumber 
/// @return 
bool Detail::Input::GetMouseRelease(uint32_t mouseButtonNumber)
{
	if (prevIsPushButton_[mouseButtonNumber] && !isPushButton_[mouseButtonNumber])
	{
		return true;
	}

	return false;
}


/// @brief マウスホイールが上開店したかどうか
/// @return 
bool Detail::Input::GetMouseWheelUp()
{
	if (static_cast<float>(mouseState_.lZ) > 0.0f)
	{
		return true;
	}

	return false;
}

/// @brief マウスホイールが下回転したかどうか
/// @return 
bool Detail::Input::GetMouseWheelDown()
{
	if (static_cast<float>(mouseState_.lZ) < 0.0f)
	{
		return true;
	}

	return false;
}


/// @brief ゲームパッドが有効かどうか
/// @param gamepadNumber 
/// @return 
bool Detail::Input::IsGamepadEnable(DWORD gamepadNumber)
{
	if (dwResult_[gamepadNumber] == ERROR_SUCCESS)
	{
		return true;
	}

	return false;
}

/// @brief ゲームパッドボタンの入力情報（Press）
/// @param gamepadNumber 
/// @param wButtons 
/// @return 
bool Detail::Input::GetGamepadButtonPress(DWORD gamepadNumber, DWORD wButtons)
{
	if (dwResult_[gamepadNumber] == ERROR_SUCCESS)
	{
		if (gamepadState_[gamepadNumber].Gamepad.wButtons & wButtons)
		{
			return true;
		}
	}

	return false;
}

/// @brief ゲームパッドボタンの入力情報（Trigger）
/// @param gamepadNumber 
/// @param wButtons 
/// @return 
bool Detail::Input::GetGamepadButtonTrigger(DWORD gamepadNumber, DWORD wButtons)
{
	if (dwResult_[gamepadNumber] == ERROR_SUCCESS)
	{
		if (!(prevGamepadState_[gamepadNumber].Gamepad.wButtons & wButtons) &&
			gamepadState_[gamepadNumber].Gamepad.wButtons & wButtons)
		{
			return true;
		}
	}

	return false;
}
/// @brief ゲームパッドボタンの入力情報（Release）
/// @param gamepadNumber 
/// @param wButtons 
/// @return 
bool Detail::Input::GetGamepadButtonRelease(DWORD gamepadNumber, DWORD wButtons)
{
	if (dwResult_[gamepadNumber] == ERROR_SUCCESS)
	{
		if (prevGamepadState_[gamepadNumber].Gamepad.wButtons & wButtons &&
			!(gamepadState_[gamepadNumber].Gamepad.wButtons & wButtons))
		{
			return true;
		}
	}

	return false;
}

/// @brief ゲームパッドの左スティックの入力情報
/// @param gamepadNumber 
/// @return 
Vector2 Detail::Input::GetGamepadLeftStick(DWORD gamepadNumber)
{
	Vector2 thumbL = { 0.0f , 0.0f };

	if (dwResult_[gamepadNumber] == ERROR_SUCCESS)
	{
		float lx = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbLX);
		float ly = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbLY);
		float length = std::sqrtf(lx * lx + ly * ly);

		if (length > static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE))
		{
			if (gamepadState_[gamepadNumber].Gamepad.sThumbLX > 0)
			{
				thumbL.x = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbLX) / SHRT_MAX;
			}
			else
			{
				thumbL.x = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbLX) / -SHRT_MIN;
			}

			if (gamepadState_[gamepadNumber].Gamepad.sThumbLY > 0)
			{
				thumbL.y = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbLY) / SHRT_MAX;
			}
			else
			{
				thumbL.y = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbLY) / -SHRT_MIN;
			}
		}
	}

	return thumbL;
}

/// @brief ゲームパッドの右スティックの入力情報
/// @param gamepadNumber 
/// @return 
Vector2 Detail::Input::GetGamepadRightStick(DWORD gamepadNumber)
{
	Vector2 thumbR = { 0.0f , 0.0f };

	if (dwResult_[gamepadNumber] == ERROR_SUCCESS)
	{
		float rx = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbRX);
		float ry = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbRY);
		float length = std::sqrtf(rx * rx + ry * ry);

		if (length > static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE))
		{
			if (gamepadState_[gamepadNumber].Gamepad.sThumbRX > 0)
			{
				thumbR.x = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbRX) / SHRT_MAX;
			}
			else
			{
				thumbR.x = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbRX) / -SHRT_MIN;
			}

			if (gamepadState_[gamepadNumber].Gamepad.sThumbRY > 0)
			{
				thumbR.y = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbRY) / SHRT_MAX;
			}
			else
			{
				thumbR.y = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.sThumbRY) / -SHRT_MIN;
			}
		}
	}

	return thumbR;
}

/// @brief ゲームパッドの左トリガーの入力情報
/// @param gamepadNumber 
/// @return 
float Detail::Input::GetGamepadLeftTrigger(DWORD gamepadNumber)
{
	float triggerL = 0.0f;

	if (dwResult_[gamepadNumber] == ERROR_SUCCESS)
	{
		triggerL = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.bLeftTrigger) / 255.0f;
	}

	return triggerL;
}

/// @brief ゲームパッドの右トリガーの入力情報
/// @param gamepadNumber 
/// @return 
float Detail::Input::GetGamepadRightTrigger(DWORD gamepadNumber)
{
	float triggerR = 0.0f;

	if (dwResult_[gamepadNumber] == ERROR_SUCCESS)
	{
		triggerR = static_cast<float>(gamepadState_[gamepadNumber].Gamepad.bRightTrigger) / 255.0f;
	}

	return triggerR;
}


/// @brief ゲームパッドを振動させる
/// @param gamepadNumber 
/// @param leftMotorPower 
/// @param rightMotorPower 
void Detail::Input::GamepadVibration(DWORD gamepadNumber, float leftMotorPower, float rightMotorPower)
{
	leftMotorPower = std::clamp(leftMotorPower, 0.0f, 1.0f);
	rightMotorPower = std::clamp(rightMotorPower, 0.0f, 1.0f);

	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));

	// モーターの強さを設定
	vibration.wLeftMotorSpeed = static_cast<WORD>(65535.0f * leftMotorPower);
	vibration.wRightMotorSpeed = static_cast<WORD>(65535.0f * rightMotorPower);

	// 振動を適用
	XInputSetState(gamepadNumber, &vibration);
}