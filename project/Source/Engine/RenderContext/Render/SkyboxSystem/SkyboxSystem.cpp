#include "SkyboxSystem.h"
#include "Engine.h"

/// @brief 初期化
void Detail::SkyboxSystem::Initialize()
{
	// Engineのインスタンスを取得する
	auto  engine = Engine::GetInstance();
	assert(engine);

	// ECSレジストリを取得する
	auto registry = engine->GetRegistryECS();
	assert(registry);

	// メンバ変数に代入する
	registry_ = registry;
}

/// @brief エンティティを作成する
/// @return 
Entity Detail::SkyboxSystem::CreateSkyboxEntity()
{
	Entity entity = registry_->CreateEntity();

	// SkyboxComponentを追加する
	SkyboxComponent skyboxComp;
	registry_->AddComponent(entity, skyboxComp);

	// CubemapComponentを追加する
	CubemapComponent cubemapComp;
	registry_->AddComponent(entity, cubemapComp);

	return entity;
}

/// @brief すべての3D描画を実行する
/// @param commandList 
/// @param priority 
void Detail::SkyboxSystem::Execute(ID3D12GraphicsCommandList* commandList)
{

}