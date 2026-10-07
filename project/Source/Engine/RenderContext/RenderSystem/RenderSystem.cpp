#include "RenderSystem.h"
#include "Engine.h"

/// @brief 初期化
void Detail::RenderSystem::Initialize()
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
Entity Detail::RenderSystem::CreateRender3DEntity()
{
	// エンティティを作成する
	Entity entity = registry_->CreateEntity();

	// RenderComponentを追加する
	RenderComponent render3DComp;
	registry_->AddComponent(entity, render3DComp);

	// TransformComponentを追加する
	TransformComponent transformComp;
	registry_->AddComponent(entity, transformComp);

	return entity;
}

/// @brief すべての3D描画を実行する
/// @param commandList 
/// @param priority 
void Detail::RenderSystem::ExecuteAll(ID3D12GraphicsCommandList* commandList, int priority)
{
	// priority が負の値の場合は何もしない
	if (priority < 0)return;

	// RenderComponentの配列を取得する
	auto* passArray = registry_->GetComponentArray<RenderComponent>();
	const auto& entities = passArray->GetEntities();

	// 今のフレームインデックスを取得する
	int32_t frameIndex = Engine::GetInstance()->GetFrameIndex();

	// priority に基づいてソートするためのペアのベクターを作成
	std::vector<std::pair<Entity, int>> sortedPasses;
	for (Entity entity : entities)
	{
		auto& pass = registry_->GetComponent<RenderComponent>(entity);
		if (pass.isEnabled && pass.priority == priority)
		{
			sortedPasses.push_back({ entity, pass.priority });
		}
	}

	// ソートされたレンダーパスが空の場合は何もしない
	if (sortedPasses.empty()) return;

	// priority に基づいて昇順でソート（数値が小さいほど先に描画）
	std::sort(sortedPasses.begin(), sortedPasses.end(), [](const auto& a, const auto& b) {return a.second < b.second;});

	// ソートされたレンダーパスを実行する
	for (const auto& [entity, priority] : sortedPasses)
	{
		
	}
}