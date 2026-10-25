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
	Engine::GetInstance()->LoadTexture("./Assets/Texture/uvChecker.png");

	// 3D描画用のエンティティを作成
	render3dEntity = Engine::GetInstance()->CreateModelEntity(Engine::GetInstance()->LoadModel("./Assets/Model/multiMaterial/multiMaterial.obj"));

	// カメラ用のエンティティを作成
	cameraEntity = Engine::GetInstance()->CreateCameraEntity();

	/// Skybox用のエンティティを作成
	skyboxEntity = Engine::GetInstance()->CreateSkyboxEntity();

	// 点光源用のエンティティを作成
	Engine::GetInstance()->CreatePointLightEntity(Vector3(0.0f, 0.0f, 15.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 2.0f, 2.0f);

	// オーディオエンティティを作成
	Engine::GetInstance()->CreateAudioEntity(Engine::GetInstance()->LoadAudio("./Assets/Sound/Undaunted_Heart.mp3"));
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