#include "Game.hpp"

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
	auto  engine = Engine::GetInstance();

	Entity renderPassEntity = engine->CreatePass(0, BlendMode::None, std::function<void()>{});

	Entity cameraEntity = engine->CreateCameraEntity();

	Entity skybox = engine->CreateSkyboxEntity();

	Entity model = engine->CreateModelEntity(engine->LoadModel("./Assets/Model/AnimatedCube/AnimatedCube.gltf"));

    Entity directionalLight = engine->CreateDirectionalLightEntity(Vector3(0.0f, 3.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, true);

	ScriptComponent scriptComp(engine->BuildScriptModule("Test", "Test", "./Assets/Script/Test.as"));
	auto registry = engine->GetRegistryECS();
	Entity scriptEntity = registry->CreateEntity();
	registry->AddComponent(scriptEntity, scriptComp);
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
