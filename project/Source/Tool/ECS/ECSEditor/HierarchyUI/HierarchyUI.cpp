#include "../ECSEditor.h"
#include "Engine.h"

void Detail::ECSEditor::DrawHierarchy()
{
#ifdef DEVELOPMENT

	ImGui::Begin("Hierarchy");

	// エンティティ作成ボタン
	if (ImGui::Button("Create Entity"))
	{
		Entity newEntity = registry_->CreateEntity();
		selectedEntity_ = newEntity;
	}

	ImGui::Separator();

	// ※ 事前にRegistryECSへ GetActiveEntities() を追加する想定
	// std::vector<Entity> を返す関数を呼び出して一覧表示します。
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
			if (ImGui::MenuItem("Destroy Entity"))
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