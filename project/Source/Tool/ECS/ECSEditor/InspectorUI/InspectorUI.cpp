#include "../ECSEditor.h"
#include "Engine.h"

void Detail::ECSEditor::DrawInspector()
{
#ifdef DEVELOPMENT

	ImGui::Begin("インスペクター");

	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();

	if (selectedEntity_ == kNullEntity)
	{
		ImGui::Text("Entityが選択されていません。");
		ImGui::End();
		return;
	}

	ImGui::Text("Entity ID: %u", selectedEntity_);
	ImGui::Separator();

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

	// RenderPassComponent
	if (registry_->HasComponent<RenderPassComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("描画パス", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& renderPass = registry_->GetComponent<RenderPassComponent>(selectedEntity_);
			ImGui::Checkbox("描画##描画パス", &renderPass.isEnabled);
			ImGui::DragInt("描画順##描画パス", &renderPass.priority, 1);
		}
	}

	// RenderComponent
	if (registry_->HasComponent<RenderComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("描画", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& render = registry_->GetComponent<RenderComponent>(selectedEntity_);
			ImGui::Checkbox("描画##描画", &render.isEnabled);
			ImGui::DragInt("描画順##描画", &render.priority, 1);
		}
	}

	// SkyboxComponent
	if (registry_->HasComponent<SkyboxComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("スカイボックス", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& skybox = registry_->GetComponent<SkyboxComponent>(selectedEntity_);
			ImGui::Checkbox("描画##スカイボックス", &skybox.isEnabled);
		}
	}

	// TransformComponent
	if (registry_->HasComponent<TransformComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("トランスフォーム", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& transform = registry_->GetComponent<TransformComponent>(selectedEntity_);
			ImGui::DragFloat3("位置##トランスフォーム", &transform.position.x, 0.1f);
			ImGui::DragFloat3("回転##トランスフォーム", &transform.rotation.x, 0.01f);
			ImGui::DragFloat3("大きさ##トランスフォーム", &transform.scale.x, 0.1f);
		}
	}

	// MaterialComponent
	if (registry_->HasComponent<MaterialComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("マテリアル", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& material = registry_->GetComponent<MaterialComponent>(selectedEntity_);
			ImGui::ColorEdit4("色##マテリアル", &material.color.x);
		}
	}

	// TextureComponent
	if (registry_->HasComponent<TextureComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("テクスチャ", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& texture = registry_->GetComponent<TextureComponent>(selectedEntity_);

			// 現在設定されているテクスチャのハンドル番号を表示
			ImGui::Text("ハンドル : %u", texture.handle);

			// ドロップを受け付けるための専用エリアを作成
			ImGui::Button("テクスチャをドロップ", ImVec2(-1, 30));

			if (ImGui::BeginDragDropTarget())
			{
				// ペイロード名 "TEXTURE_HANDLE" を受け取る
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_HANDLE"))
				{
					IM_ASSERT(payload->DataSize == sizeof(uint32_t));
					uint32_t droppedHandle = *(const uint32_t*)payload->Data;

					if (engine)
					{
						if (auto textureStore = engine->GetRenderContext()->GetTextureStore())
						{
							// テクスチャ2Dのみ変更する
							if(textureStore->GetType(droppedHandle) == TextureType::Texture2D)
								texture.handle = droppedHandle;
						}

					}
				}
				ImGui::EndDragDropTarget();
			}
		}
	}

	// CubemapComponent
	if (registry_->HasComponent<CubemapComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("キューブマップ", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& texture = registry_->GetComponent<CubemapComponent>(selectedEntity_);

			// 現在設定されているキューブマップのハンドル番号を表示
			ImGui::Text("ハンドル : %u", texture.handle);

			// ドロップを受け付けるための専用エリアを作成
			ImGui::Button("キューブマップをドロップ", ImVec2(-1, 30));

			if (ImGui::BeginDragDropTarget())
			{
				// ペイロード名 "TEXTURE_HANDLE" を受け取る
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_HANDLE"))
				{
					IM_ASSERT(payload->DataSize == sizeof(uint32_t));
					uint32_t droppedHandle = *(const uint32_t*)payload->Data;

					if (engine)
					{
						if (auto textureStore = engine->GetRenderContext()->GetTextureStore())
						{
							// キューブマップのみ変更する
							if (textureStore->GetType(droppedHandle) == TextureType::Cubemap)
								texture.handle = droppedHandle;
						}

					}
				}
				ImGui::EndDragDropTarget();
			}
		}
	}

	// UVTransformComponent
	if (registry_->HasComponent<UVTransformComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("UVトランスフォーム", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& uvTransform = registry_->GetComponent<UVTransformComponent>(selectedEntity_);
			ImGui::DragFloat2("オフセット##UV", &uvTransform.offset.x, 0.1f);
			ImGui::DragFloat("回転##UV", &uvTransform.radian, 0.01f);
			ImGui::DragFloat2("大きさ##UV", &uvTransform.scale.x, 0.1f);
		}
	}

	// DirectionalLightComponent
	if (registry_->HasComponent<DirectionalLightComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("平行光源", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& directionalLight = registry_->GetComponent<DirectionalLightComponent>(selectedEntity_);
			ImGui::Checkbox("有効##平行光源", &directionalLight.isEnabled);
			if (ImGui::DragFloat3("方向##平行光源", &directionalLight.direction.x, 0.1f))
			{
				// 入力された方向ベクトルを正規化する
				directionalLight.direction = directionalLight.direction.Normalize();
			}
			ImGui::ColorEdit4("色##平行光源", &directionalLight.color.x);
			ImGui::DragFloat("強さ##平行光源", &directionalLight.intensity, 0.1f);
		}
	}

	// PointLightComponent
	if (registry_->HasComponent<PointLightComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("ポイントライト", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& pointLight = registry_->GetComponent<PointLightComponent>(selectedEntity_);
			ImGui::Checkbox("有効##ポイントライト", &pointLight.isEnabled);
			ImGui::ColorEdit4("色##ポイントライト", &pointLight.color.x);
			ImGui::DragFloat("強さ##ポイントライト", &pointLight.intensity, 0.1f);
			ImGui::DragFloat("半径##ポイントライト", &pointLight.radius, 0.1f);
			ImGui::DragFloat("減衰##ポイントライト", &pointLight.decay, 0.1f);
		}
	}

	// SpotLightComponent
	if (registry_->HasComponent<SpotLightComponent>(selectedEntity_))
	{
		if (ImGui::CollapsingHeader("スポットライト", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& spotLight = registry_->GetComponent<SpotLightComponent>(selectedEntity_);
			ImGui::Checkbox("有効##スポットライト", &spotLight.isEnabled);
			if (ImGui::DragFloat3("方向##スポットライト", &spotLight.direction.x, 0.1f))
			{
				// 入力された方向ベクトルを正規化する
				spotLight.direction = spotLight.direction.Normalize();
			}
			ImGui::ColorEdit4("色##スポットライト", &spotLight.color.x);
			ImGui::DragFloat("強さ##スポットライト", &spotLight.intensity, 0.1f);
			ImGui::DragFloat("半径##スポットライト", &spotLight.distance, 0.1f);
			ImGui::DragFloat("減衰##スポットライト", &spotLight.decay, 0.1f);
			ImGui::DragFloat("角度##スポットライト", &spotLight.cosAngle, 0.01f);
			ImGui::DragFloat("減衰開始角度##スポットライト", &spotLight.cosFalloffStart, 0.01f);
		}
	}

	ImGui::End();

#endif
}