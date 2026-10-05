#include "Game.h"

/// @brief コンストラクタ
/// @param screenWidth スクリーン横幅
/// @param screenHeight スクリーン縦幅
/// @param title タイトル
Game::Game(int32_t screenWidth, int32_t screenHeight, const std::string& title)
{
	// エンジンを取得
	Engine::GetInstance(screenWidth, screenHeight, title);
}

/// @brief 初期化
void Game::Initialize()
{
	// レンダーパスを作成
	renderPassEntity = Engine::GetInstance()->CreatePass(0, BlendMode::None, [this]() {});

	// テクスチャを読み込む
	textureHandle = Engine::GetInstance()->LoadTexture("./Assets/Texture/uvChecker.png");
}

/// @brief 更新処理
void Game::Update()
{

}

/// @brief 実行
/// @return 
int32_t Game::Run()
{
	// エンジンを取得
	Engine* engine = Engine::GetInstance();

	// 初期化
	Initialize();

	// ゲームループ
	while (engine->GameLoop())
	{
		// 新フレーム処理
		engine->NewFrame();

		// 更新処理
		Update();

		// 描画後処理
		engine->PostDraw();
	}

	return 0;
}