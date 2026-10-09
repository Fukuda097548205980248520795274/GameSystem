#include "../ECSEditor.h"
#include "Engine.h"

void Detail::ECSEditor::DrawInspector()
{
	ImGui::Begin("Inspector");

	if (selectedEntity_ == kNullEntity)
	{
		ImGui::Text("No entity selected.");
		ImGui::End();
		return;
	}

	ImGui::Text("Entity ID: %u", selectedEntity_);
	ImGui::Separator();

	// TransformComponent
	if (registry_->HasComponent<TransformComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& transform = registry_->GetComponent<TransformComponent>(selectedEntity_);
			ImGui::DragFloat3("Position", &transform.position.x, 0.1f);
			ImGui::DragFloat3("Rotation", &transform.rotation.x, 0.1f);
			ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f);
		}
	}
	else
	{
		if (ImGui::Button("Add Transform"))
			registry_->AddComponent<TransformComponent>(selectedEntity_, TransformComponent());
	}

	// BlendComponent
	if (registry_->HasComponent<BlendComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("Blend", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& blend = registry_->GetComponent<BlendComponent>(selectedEntity_);
			ImGui::SliderFloat("Opacity", &blend.opacity, 0.0f, 1.0f);

			const char* blendModes[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
			int currentMode = static_cast<int>(blend.blendMode);
			if (ImGui::Combo("Blend Mode", &currentMode, blendModes, IM_ARRAYSIZE(blendModes)))
			{
				blend.blendMode = static_cast<BlendMode>(currentMode);
			}
		}
	}
	else
	{
		if (ImGui::Button("Add Blend"))
			registry_->AddComponent<BlendComponent>(selectedEntity_, BlendComponent());
	}

	// RenderComponent
	if (registry_->HasComponent<RenderComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& render = registry_->GetComponent<RenderComponent>(selectedEntity_);
			ImGui::Checkbox("Is Enabled", &render.isEnabled);
			ImGui::DragInt("Priority", &render.priority, 1);
		}
	}
	else
	{
		if (ImGui::Button("Add Render"))
			registry_->AddComponent<RenderComponent>(selectedEntity_, RenderComponent());
	}

	ImGui::End();
}