#pragma once
#include "Engine.hpp"

/// @brief ゲーム
class Game
{
public:

	/// @brief コンストラクタ
	~Game() = default;

	/// @brief コンストラクタ
	/// @param screenWidth スクリーン横幅
	/// @param screenHeight スクリーン縦幅
	/// @param title タイトル
	Game(int32_t screenWidth, int32_t screenHeight, const std::string& title);

	/// @brief 実行
	/// @return 
	int32_t Run();


protected:

	/// @brief 初期化
	void Initialize();

	/// @brief 更新処理
	void Update();
};

