#include "../ECSEditor.h"
#include "Engine.h"

/// @brief インスペクターUIを描画する
void Detail::ECSEditor::DrawInspector()
{
#ifdef DEVELOPMENT

	ImGui::Begin("インスペクター");

	if (selectedEntity_ == kNullEntity)
	{
		ImGui::Text("Entityが選択されていません。");
		ImGui::End();
		return;
	}

	ImGui::Text("Selected Entity ID: %u", selectedEntity_);
	ImGui::Separator();

	// 選択中のエンティティをルートとして再帰的にコンポーネントを描画
	DrawEntityComponents(selectedEntity_);

	ImGui::End();

#endif
}

/// @brief エンティティコンポーネントUIの描画
void Detail::ECSEditor::DrawEntityComponents(Entity entity)
{
	auto engine = Engine::GetInstance();

	// TreeNodeなどを使ってエンティティごとの階層を視覚的に分ける
	std::string nodeName = "Entity " + std::to_string(entity);
	if (ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		// BlendComponent
		if (registry_->HasComponent<BlendComponent>(entity))
		{
			if (ImGui::CollapsingHeader("Blend", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto blend = registry_->GetComponent<BlendComponent>(entity);
				if (blend)
				{
					ImGui::SliderFloat("Opacity", &blend->opacity, 0.0f, 1.0f);

					const char* blendModes[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
					int currentMode = static_cast<int>(blend->blendMode);
					if (ImGui::Combo("Blend Mode", &currentMode, blendModes, IM_ARRAYSIZE(blendModes)))
					{
						blend->blendMode = static_cast<BlendMode>(currentMode);
					}
				}
			}
		}

		// RenderPassComponent
		if (registry_->HasComponent<RenderPassComponent>(entity))
		{
			if (ImGui::CollapsingHeader("描画パス", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto renderPass = registry_->GetComponent<RenderPassComponent>(entity);
				if (renderPass)
				{
					ImGui::Checkbox("描画##描画パス", &renderPass->isEnabled);
					ImGui::DragInt("描画順##描画パス", &renderPass->priority, 1);
				}
			}
		}

		// RenderComponent
		if (registry_->HasComponent<RenderComponent>(entity))
		{
			if (ImGui::CollapsingHeader("描画", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto render = registry_->GetComponent<RenderComponent>(entity);
				if (render)
				{
					ImGui::Checkbox("描画##描画", &render->isEnabled);
					ImGui::DragInt("描画順##描画", &render->priority, 1);
				}
			}
		}

		// SkyboxComponent
		if (registry_->HasComponent<SkyboxComponent>(entity))
		{
			if (ImGui::CollapsingHeader("スカイボックス", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto skybox = registry_->GetComponent<SkyboxComponent>(entity);
				if (skybox)
				{
					ImGui::Checkbox("描画##スカイボックス", &skybox->isEnabled);
				}
			}
		}

		// TransformComponent
		if (registry_->HasComponent<TransformComponent>(entity))
		{
			if (ImGui::CollapsingHeader("トランスフォーム", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto transform = registry_->GetComponent<TransformComponent>(entity);
				if (transform)
				{
					ImGui::DragFloat3("位置##トランスフォーム", &transform->position.x, 0.01f);
					ImGui::DragFloat3("回転##トランスフォーム", &transform->rotation.x, 0.01f);
					ImGui::DragFloat3("大きさ##トランスフォーム", &transform->scale.x, 0.1f);
				}
			}
		}

		// MaterialComponent
		if (registry_->HasComponent<MaterialComponent>(entity))
		{
			if (ImGui::CollapsingHeader("マテリアル", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto material = registry_->GetComponent<MaterialComponent>(entity);
				if (material)
				{
					ImGui::ColorEdit4("色##マテリアル", &material->color.x);
				}
			}
		}

		// TextureComponent
		if (registry_->HasComponent<TextureComponent>(entity))
		{
			if (ImGui::CollapsingHeader("テクスチャ", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto texture = registry_->GetComponent<TextureComponent>(entity);
				if (texture)
				{
					// 現在設定されているテクスチャのハンドル番号を表示
					ImGui::Text("ハンドル : Index(%u), Gen(%u)", texture->handle.index, texture->handle.generation);

					// ドロップを受け付けるための専用エリアを作成
					ImGui::Button("テクスチャをドロップ", ImVec2(-1, 30));

					if (ImGui::BeginDragDropTarget())
					{
						// ペイロード名 "TEXTURE_HANDLE" を受け取る
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_HANDLE"))
						{
							IM_ASSERT(payload->DataSize == sizeof(Detail::TextureHandle));
							Detail::TextureHandle droppedHandle = *(const Detail::TextureHandle*)payload->Data;

							if (engine)
							{
								if (auto textureStore = engine->GetRenderContext()->GetTextureStore())
								{
									// テクスチャ2Dのみ変更する
									if (textureStore->GetType(droppedHandle) == TextureType::Texture2D)
										texture->handle = droppedHandle;
								}

							}
						}
						ImGui::EndDragDropTarget();
					}
				}
			}
		}

		// CubemapComponent
		if (registry_->HasComponent<CubemapComponent>(entity))
		{
			if (ImGui::CollapsingHeader("キューブマップ", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto texture = registry_->GetComponent<CubemapComponent>(entity);
				if (texture)
				{
					// 現在設定されているキューブマップのハンドル番号を表示
					ImGui::Text("ハンドル : Index(%u), Gen(%u)", texture->handle.index, texture->handle.generation);

					// ドロップを受け付けるための専用エリアを作成
					ImGui::Button("キューブマップをドロップ", ImVec2(-1, 30));

					if (ImGui::BeginDragDropTarget())
					{
						// ペイロード名 "TEXTURE_HANDLE" を受け取る
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_HANDLE"))
						{
							IM_ASSERT(payload->DataSize == sizeof(Detail::TextureHandle));
							Detail::TextureHandle droppedHandle = *(const Detail::TextureHandle*)payload->Data;

							if (engine)
							{
								if (auto textureStore = engine->GetRenderContext()->GetTextureStore())
								{
									// キューブマップのみ変更する
									if (textureStore->GetType(droppedHandle) == TextureType::Cubemap)
										texture->handle = droppedHandle;
								}

							}
						}
						ImGui::EndDragDropTarget();
					}
				}
			}
		}

		// ModelComponent
		if (registry_->HasComponent<ModelComponent>(entity))
		{
			if (ImGui::CollapsingHeader("モデル", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto model = registry_->GetComponent<ModelComponent>(entity);
				if (model)
				{
					ImGui::Text("ハンドル : Index(%u), Gen(%u)", model->handle.index, model->handle.generation);

					// ドロップを受け付けるための専用エリアを作成
					ImGui::Button("モデルをドロップ", ImVec2(-1, 30));

					if (ImGui::BeginDragDropTarget())
					{
						// ペイロード名 "MODEL_HANDLE" を受け取る
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODEL_HANDLE"))
						{
							IM_ASSERT(payload->DataSize == sizeof(Detail::ModelHandle));
							Detail::ModelHandle droppedHandle = *(const Detail::ModelHandle*)payload->Data;

							if (engine)
							{
								if (auto modelStore = engine->GetRenderContext()->GetModelStore())
								{
									// モデルハンドルを変更する
									model->handle = droppedHandle;
								}
							}
						}
						ImGui::EndDragDropTarget();
					}
				}
			}
		}

		// UVTransformComponent
		if (registry_->HasComponent<UVTransformComponent>(entity))
		{
			if (ImGui::CollapsingHeader("UVトランスフォーム", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto uvTransform = registry_->GetComponent<UVTransformComponent>(entity);
				if (uvTransform)
				{
					ImGui::DragFloat2("オフセット##UV", &uvTransform->offset.x, 0.1f);
					ImGui::DragFloat("回転##UV", &uvTransform->radian, 0.01f);
					ImGui::DragFloat2("大きさ##UV", &uvTransform->scale.x, 0.1f);
				}
			}
		}

		// DirectionalLightComponent
		if (registry_->HasComponent<DirectionalLightComponent>(entity))
		{
			if (ImGui::CollapsingHeader("平行光源", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto directionalLight = registry_->GetComponent<DirectionalLightComponent>(entity);
				if (directionalLight)
				{
					ImGui::Checkbox("有効##平行光源", &directionalLight->isEnabled);
					if (ImGui::DragFloat3("方向##平行光源", &directionalLight->direction.x, 0.1f))
					{
						// 入力された方向ベクトルを正規化する
						directionalLight->direction = directionalLight->direction.Normalize();
					}
					ImGui::ColorEdit3("色##平行光源", &directionalLight->color.x);
					ImGui::DragFloat("強さ##平行光源", &directionalLight->intensity, 0.1f);
				}
			}
		}

		// PointLightComponent
		if (registry_->HasComponent<PointLightComponent>(entity))
		{
			if (ImGui::CollapsingHeader("ポイントライト", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto pointLight = registry_->GetComponent<PointLightComponent>(entity);
				if (pointLight)
				{
					ImGui::Checkbox("有効##ポイントライト", &pointLight->isEnabled);
					ImGui::DragFloat3("位置##ポイントライト", &pointLight->position.x, 0.01f);
					ImGui::ColorEdit3("色##ポイントライト", &pointLight->color.x);
					ImGui::DragFloat("強さ##ポイントライト", &pointLight->intensity, 0.1f);
					ImGui::DragFloat("半径##ポイントライト", &pointLight->radius, 0.1f);
					ImGui::DragFloat("減衰##ポイントライト", &pointLight->decay, 0.1f);
				}
			}
		}

		// SpotLightComponent
		if (registry_->HasComponent<SpotLightComponent>(entity))
		{
			if (ImGui::CollapsingHeader("スポットライト", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto spotLight = registry_->GetComponent<SpotLightComponent>(entity);
				if (spotLight)
				{
					ImGui::Checkbox("有効##スポットライト", &spotLight->isEnabled);
					ImGui::DragFloat3("位置##スポットライト", &spotLight->position.x, 0.01f);
					if (ImGui::DragFloat3("方向##スポットライト", &spotLight->direction.x, 0.1f))
					{
						// 入力された方向ベクトルを正規化する
						spotLight->direction = spotLight->direction.Normalize();
					}
					ImGui::ColorEdit3("色##スポットライト", &spotLight->color.x);
					ImGui::DragFloat("強さ##スポットライト", &spotLight->intensity, 0.1f);
					ImGui::DragFloat("半径##スポットライト", &spotLight->distance, 0.1f);
					ImGui::DragFloat("減衰##スポットライト", &spotLight->decay, 0.1f);
					ImGui::DragFloat("角度##スポットライト", &spotLight->cosAngle, 0.01f);
					ImGui::DragFloat("減衰開始角度##スポットライト", &spotLight->cosFalloffStart, 0.01f);
				}
			}
		}

		// InputComponent
		if (registry_->HasComponent<InputComponent>(entity))
		{
			if (ImGui::CollapsingHeader("入力", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto input = registry_->GetComponent<InputComponent>(entity);
				if (input)
				{
					// 1. デバイスの切り替え
					const char* deviceNames[] = { "キーボード＆マウス", "ゲームパッド" };
					int currentDevice = static_cast<int>(input->deviceType);
					if (ImGui::Combo("入力デバイス##入力", &currentDevice, deviceNames, IM_ARRAYSIZE(deviceNames)))
					{
						input->deviceType = static_cast<InputDeviceType>(currentDevice);
					}

					ImGui::Separator();

					// 2. 選択されたデバイスに応じた割り当て設定
					if (input->deviceType == InputDeviceType::KeyboardMouse)
					{
						ImGui::Text("【キーボード割り当て】");

						// キー割り当てUIを描画・処理するためのラムダ式
						auto DrawKeyBindUI = [&](const char* label, uint32_t& key, int id)
							{
								ImGui::PushID(id);

								ImGui::Text("%s: 0x%02X", label, key);
								ImGui::SameLine(150.0f); // ボタンの位置を右に揃える

								// 変更ボタンを押したらポップアップを開く
								if (ImGui::Button("キーを変更"))
								{
									ImGui::OpenPopup("BindPopup");
								}

								// ポップアップ内の処理（キー入力待機）
								if (ImGui::BeginPopupModal("BindPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
								{
									ImGui::Text("割り当てたいキーを押してください...");
									ImGui::TextDisabled("※キャンセルする場合は ESCキー");
									ImGui::Separator();

									if (ImGui::Button("キャンセル"))
									{
										ImGui::CloseCurrentPopup();
									}

									if (engine && engine->GetInput())
									{
										// ESCキー (DIK_ESCAPE = 0x01) が押されたらキャンセル
										if (engine->GetInput()->GetKeyTrigger(0x01))
										{
											ImGui::CloseCurrentPopup();
										}
										else
										{
											// 0x02 〜 0xFF の全キーをループして押されたか判定
											for (BYTE i = 0x02; i < 0xFF; ++i)
											{
												if (engine->GetInput()->GetKeyTrigger(i))
												{
													key = i; // 新しいキーコードを割り当て
													ImGui::CloseCurrentPopup(); // ポップアップを閉じる
													break;
												}
											}
										}
									}
									ImGui::EndPopup();
								}
								ImGui::PopID();
							};

						DrawKeyBindUI("前移動キー", input->moveUpKey, 0);
						DrawKeyBindUI("後移動キー", input->moveDownKey, 1);
						DrawKeyBindUI("左移動キー", input->moveLeftKey, 2);
						DrawKeyBindUI("右移動キー", input->moveRightKey, 3);
						DrawKeyBindUI("アクション", input->actionKey, 4);
					}
					else if (input->deviceType == InputDeviceType::Gamepad)
					{
						ImGui::Text("【ゲームパッド割り当て】");
						ImGui::DragInt("コントローラー番号##入力", &input->gamepadNumber, 1, 0, 3);

						// XINPUTボタンをコンボボックスで選択しやすくする
						const char* buttonNames[] = { "Aボタン", "Bボタン", "Xボタン", "Yボタン", "START", "BACK" };
						uint32_t buttonFlags[] = { 0x1000, 0x2000, 0x4000, 0x8000, 0x0010, 0x0020 }; // XINPUT_GAMEPAD_* の値

						int currentButtonIndex = 0;
						for (int i = 0; i < IM_ARRAYSIZE(buttonFlags); ++i)
						{
							if (input->actionButton == buttonFlags[i])
							{
								currentButtonIndex = i;
								break;
							}
						}

						if (ImGui::Combo("アクションボタン##入力", &currentButtonIndex, buttonNames, IM_ARRAYSIZE(buttonNames)))
						{
							input->actionButton = buttonFlags[currentButtonIndex];
						}
					}

					ImGui::Separator();

					// 3. 実際の入力状態の可視化（デバッグ・疑似操作用）
					ImGui::Text("【現在の入力状態（デバッグ用）】");
					ImGui::DragFloat2("移動入力##入力", &input->moveInput.x, 0.01f);
					ImGui::DragFloat2("視点入力##入力", &input->cameraInput.x, 0.01f);
					ImGui::Checkbox("アクション押下##入力", &input->isActionPressed);
					ImGui::Checkbox("アクショントリガー##入力", &input->isActionTriggered);
				}
			}
		}

		// HierarchyComponent
		if (registry_->HasComponent<HierarchyComponent>(entity))
		{
			auto hierarchy = registry_->GetComponent<HierarchyComponent>(entity);
			if (hierarchy && !hierarchy->children.empty())
			{
				ImGui::Separator();
				ImGui::Text("Children");

				// 子エンティティに対して再帰処理を行う
				for (Entity child : hierarchy->children)
				{
					DrawEntityComponents(child);
				}
			}
		}

		ImGui::TreePop();
	}
}