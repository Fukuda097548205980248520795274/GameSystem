#include "../ECSEditor.h"
#include "Engine.h"

//// @brief ヒエラルキーUIを描画する
void Detail::ECSEditor::DrawHierarchy()
{
#ifdef DEVELOPMENT

	ImGui::Begin("ヒエラルキー");

	// エンティティ作成ボタン
	if (ImGui::Button("Entity作成"))
	{
		Entity newEntity = registry_->CreateEntity();
		selectedEntity_ = newEntity;
	}

	ImGui::Separator();

	// 有効なエンティティの一覧を取得
	const auto& entities = registry_->GetActiveEntities();
	for (Entity entity : entities)
	{
		// エンティティ名（現状はIDで代用）
		std::string label = "Entity " + std::to_string(entity);

		// 選択状態の判定とUI描画
		bool isSelected = (selectedEntity_ == entity);
		if (ImGui::Selectable(label.c_str(), isSelected))
		{
			selectedEntity_ = entity;
		}

		// 右クリックでコンテキストメニューを表示
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Entityを破棄"))
			{
				registry_->DestroyEntity(entity);
				if (selectedEntity_ == entity)
				{
					selectedEntity_ = kNullEntity;
				}
			}
			ImGui::EndPopup();
		}
	}

	ImGui::End();

#endif
}