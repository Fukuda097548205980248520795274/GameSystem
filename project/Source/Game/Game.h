#pragma once
#include "Engine.h"

class Game
{
public:

	/// @brief コンストラクタ
	virtual ~Game() = default;

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


private:

	/// @brief レンダーパスのエンティティ
	Entity renderPassEntity;

	/// @brief 読み込んだテクスチャのハンドル
	uint32_t textureHandle = 0;

	/// @brief 3D描画用のエンティティ
	Entity render3dEntity;

	/// @brief カメラ用のエンティティ
	Entity cameraEntity;

	/// @brief Skybox用のエンティティ
	Entity skyboxEntity;
};

