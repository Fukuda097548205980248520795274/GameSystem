#include "ECSEditor.h"
#include "Engine.h"

/// @brief 初期化
void Detail::ECSEditor::Initialize()
{
	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();
	assert(engine != nullptr);

	// ECSレジストリを取得
	registry_ = engine->GetRegistryECS();
	assert(registry_ != nullptr);

	// 選択中のエンティティを初期化
	selectedEntity_ = kNullEntity;
}

/// @brief 更新
void Detail::ECSEditor::Update()
{
	// UIウィンドウの描画
	DrawHierarchy();
	DrawInspector();
}