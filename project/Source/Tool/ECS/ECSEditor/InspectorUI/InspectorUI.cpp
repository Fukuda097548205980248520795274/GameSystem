#include "../ECSEditor.hpp"
#include "Engine.hpp"

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

	ImGui::Text("選択中 Entity ID: %u", selectedEntity_);
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

        // TagComponent
        DrawComponent<TagComponent>(entity, "タグ", [](TagComponent* tagComp)
            {
                char buffer[256];
                strncpy_s(buffer, tagComp->tag.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';

                ImGui::Text("タグ名");
                ImGui::SameLine(100.0f);
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
                {
                    tagComp->tag = buffer;
                }
            });

        // ScriptComponent
        DrawComponent<ScriptComponent>(entity, "スクリプト", [this, engine](ScriptComponent* script)
            {
                // スクリプトの割り当てUI（Unityのスクリプトスロット風）
                ImGui::Text("スクリプト");
                ImGui::SameLine(100.0f);
                std::string scriptName = script->className.empty() ? "None (Script)" : script->className;
                ImGui::Button(scriptName.c_str(), ImVec2(-1, 0));

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCRIPT_HANDLE"))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(Detail::ScriptHandle));
                        Detail::ScriptHandle droppedHandle = *(const Detail::ScriptHandle*)payload->Data;

                        if (engine)
                        {
                            if (auto scriptManager = engine->GetScriptSystem()->GetScriptManager())
                            {
                                script->handle = droppedHandle;
                                script->filePath = scriptManager->GetFilePath(droppedHandle);
                                script->moduleName = scriptManager->GetModuleName(droppedHandle);
                                script->className = scriptManager->GetScriptClass(droppedHandle);
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::Separator();

                // 変数の自動露出（UnityのPublic変数表示）
                if (engine && !script->moduleName.empty() && !script->className.empty())
                {
                    auto scriptManager = engine->GetScriptSystem()->GetScriptManager();
                    auto scriptEngine = scriptManager->GetScriptEngine();

                    asIScriptModule* module = scriptEngine->GetModule(script->moduleName.c_str());
                    if (module)
                    {
                        asITypeInfo* typeInfo = module->GetTypeInfoByName(script->className.c_str());
                        if (typeInfo)
                        {
                            for (asUINT i = 0; i < typeInfo->GetPropertyCount(); i++)
                            {
                                const char* propName;
                                int typeId;
                                typeInfo->GetProperty(i, &propName, &typeId, nullptr, nullptr, nullptr, nullptr, nullptr);

                                // ゲーム実行中の場合は、メモリ上の実アドレスを取得して直接書き換える
                                void* livePropAddr = script->scriptObject ? script->scriptObject->GetAddressOfProperty(i) : nullptr;

                                // 変数名を表示
                                ImGui::Text("%s", propName);
                                ImGui::SameLine(100.0f);
                                ImGui::PushID(i);

                                // --- 整数型 (int) ---
                                if (typeId == asTYPEID_INT32)
                                {
                                    int val = livePropAddr ? *(int*)livePropAddr : script->savedIntProperties[propName];
                                    ImGui::SetNextItemWidth(-1);
                                    if (ImGui::DragInt("##val", &val))
                                    {
                                        script->savedIntProperties[propName] = val;
                                        if (livePropAddr) *(int*)livePropAddr = val;
                                    }
                                }
                                // --- 浮動小数点型 (float) ---
                                else if (typeId == asTYPEID_FLOAT)
                                {
                                    float val = livePropAddr ? *(float*)livePropAddr : script->savedFloatProperties[propName];
                                    ImGui::SetNextItemWidth(-1);
                                    if (ImGui::DragFloat("##val", &val, 0.1f))
                                    {
                                        script->savedFloatProperties[propName] = val;
                                        if (livePropAddr) *(float*)livePropAddr = val;
                                    }
                                }
                                // --- 真偽値 (bool) ---
                                else if (typeId == asTYPEID_BOOL)
                                {
                                    bool val = livePropAddr ? *(bool*)livePropAddr : script->savedBoolProperties[propName];
                                    if (ImGui::Checkbox("##val", &val))
                                    {
                                        script->savedBoolProperties[propName] = val;
                                        if (livePropAddr) *(bool*)livePropAddr = val;
                                    }
                                }
                                // --- Vector2 ---
                                else if (typeId == scriptEngine->GetTypeIdByDecl("Vector2"))
                                {
                                    Vector2 val = livePropAddr ? *(Vector2*)livePropAddr : script->savedVector2Properties[propName];
                                    ImGui::SetNextItemWidth(-1);
                                    // 配列として先頭アドレス（&val.x）を渡す
                                    if (ImGui::DragFloat2("##val", &val.x, 0.1f))
                                    {
                                        script->savedVector2Properties[propName] = val;
                                        if (livePropAddr) *(Vector2*)livePropAddr = val;
                                    }
                                }
                                // --- Vector3 ---
                                else if (typeId == scriptEngine->GetTypeIdByDecl("Vector3"))
                                {
                                    Vector3 val = livePropAddr ? *(Vector3*)livePropAddr : script->savedVector3Properties[propName];
                                    ImGui::SetNextItemWidth(-1);
                                    if (ImGui::DragFloat3("##val", &val.x, 0.1f))
                                    {
                                        script->savedVector3Properties[propName] = val;
                                        if (livePropAddr) *(Vector3*)livePropAddr = val;
                                    }
                                }
                                // --- String ---
                                else if (typeId == scriptEngine->GetTypeIdByDecl("string"))
                                {
                                    std::string val = livePropAddr ? *(std::string*)livePropAddr : script->savedStringProperties[propName];

                                    // ImGui用のバッファを用意
                                    char buffer[256];
                                    strncpy_s(buffer, val.c_str(), sizeof(buffer) - 1);
                                    buffer[sizeof(buffer) - 1] = '\0'; // 終端を保証

                                    ImGui::SetNextItemWidth(-1);
                                    if (ImGui::InputText("##val", buffer, sizeof(buffer)))
                                    {
                                        std::string newVal(buffer);
                                        script->savedStringProperties[propName] = newVal;
                                        if (livePropAddr) *(std::string*)livePropAddr = newVal;
                                    }
                                }
                                // --- Entity型 (UnityのGameObjectスロット風) ---
                                else if (typeId == scriptEngine->GetTypeIdByDecl("Entity"))
                                {
                                    Entity currentEntity = livePropAddr ? *(Entity*)livePropAddr :
                                        (script->savedEntityProperties.count(propName) ? script->savedEntityProperties[propName] : kNullEntity);

                                    std::string btnLabel = currentEntity != kNullEntity ? "Entity " + std::to_string(currentEntity) : "None (Entity)";
                                    ImGui::Button(btnLabel.c_str(), ImVec2(-1, 0));

                                    if (ImGui::BeginDragDropTarget())
                                    {
                                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID"))
                                        {
                                            Entity droppedEntity = *(const Entity*)payload->Data;
                                            script->savedEntityProperties[propName] = droppedEntity;
                                            if (livePropAddr) *(Entity*)livePropAddr = droppedEntity;
                                        }
                                        ImGui::EndDragDropTarget();
                                    }
                                }
                                else
                                {
                                    ImGui::TextDisabled("Unsupported Type");
                                }

                                ImGui::PopID();
                            }
                        }
                    }
                }
            });


        // BlendComponent
        DrawComponent<BlendComponent>(entity, "ブレンド", [this, engine](BlendComponent* blend)
            {
                ImGui::SliderFloat("Opacity", &blend->opacity, 0.0f, 1.0f);

                const char* blendModes[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
                int currentMode = static_cast<int>(blend->blendMode);
                if (ImGui::Combo("Blend Mode", &currentMode, blendModes, IM_ARRAYSIZE(blendModes)))
                {
                    blend->blendMode = static_cast<BlendMode>(currentMode);
                }
            });

        // RenderPassComponent
        DrawComponent<RenderPassComponent>(entity, "描画パス", [this, engine](RenderPassComponent* renderPass)
            {
                ImGui::Checkbox("描画##描画パス", &renderPass->isEnabled);
                ImGui::DragInt("描画順##描画パス", &renderPass->priority, 1);
            });

        // RenderComponent
        DrawComponent<RenderComponent>(entity, "描画", [this, engine](RenderComponent* render)
            {
                ImGui::Checkbox("描画##描画", &render->isEnabled);
                ImGui::DragInt("描画順##描画", &render->priority, 1);
            });

        // InstanceCountComponent
        DrawComponent<InstanceCountComponent>(entity, "インスタンス数", [this, engine](InstanceCountComponent* instanceCount)
            {
                ImGui::Text("インスタンス数##インスタンス数 : %d", &instanceCount->count);
            });

        // SkyboxComponent
        DrawComponent<SkyboxComponent>(entity, "スカイボックス", [this, engine](SkyboxComponent* skybox)
            {
                ImGui::Checkbox("描画##スカイボックス", &skybox->isEnabled);
            });

        // TransformComponent
        DrawComponent<TransformComponent>(entity, "トランスフォーム", [this, engine](TransformComponent* transform)
            {
                ImGui::DragFloat3("位置##トランスフォーム", &transform->position.x, 0.01f);
                ImGui::DragFloat3("回転##トランスフォーム", &transform->rotation.x, 0.01f);
                ImGui::DragFloat3("大きさ##トランスフォーム", &transform->scale.x, 0.1f);
            });

        // CameraComponent
        DrawComponent<CameraComponent>(entity, "カメラ", [this, engine](CameraComponent* camera)
            {
                // 投影モードの選択
                const char* projectionModes[] = { "平行投影（2D）", "透視投影（3D）" };
                int currentMode = static_cast<int>(camera->projectionMode);
                if (ImGui::Combo("投影モード", &currentMode, projectionModes, IM_ARRAYSIZE(projectionModes)))
                {
                    camera->projectionMode = static_cast<ProjectionMode>(currentMode);
                }
                // 投影パラメータの表示
                if (camera->projectionMode == ProjectionMode::Perspective)
                {
                    ImGui::DragFloat("垂直視野角 (fovY)", &camera->fovY, 0.01f);
                    ImGui::DragFloat("アスペクト比", &camera->aspectRatio, 0.01f);
                    ImGui::DragFloat("近平面距離", &camera->nearPlane, 0.01f);
                    ImGui::DragFloat("遠平面距離", &camera->farPlane, 0.01f);
                }
                else if (camera->projectionMode == ProjectionMode::Orthographic)
                {
                    ImGui::DragFloat("平行投影サイズ", &camera->orthographicSize, 0.01f);
                    ImGui::DragFloat("遠平面距離", &camera->farPlane, 0.01f);
                }
            });

        // PivotPointComponent
        DrawComponent<PivotPointComponent>(entity, "ピボットポイント", [this, engine](PivotPointComponent* pivot)
            {
                ImGui::DragFloat3("ピボット位置", &pivot->pivotPoint.x, 0.01f);
                ImGui::DragFloat("長さ", &pivot->length, 0.01f);
                ImGui::DragFloat("回転角度 Theta", &pivot->theta, 0.01f);
                ImGui::DragFloat("回転角度 Phi", &pivot->phi, 0.01f);

                // 球面座標からデカルト座標への変換結果を表示
                Vector3 cartesian = pivot->GetSphericalCoordinates();
                ImGui::Text("デカルト座標: (%.2f, %.2f, %.2f)", cartesian.x, cartesian.y, cartesian.z);
            });

        // MaterialComponent
        DrawComponent<MaterialComponent>(entity, "マテリアル", [this, engine](MaterialComponent* material)
            {
                ImGui::ColorEdit4("色##マテリアル", &material->color.x);
            });

        // AnimationComponent
        DrawComponent<AnimationComponent>(entity, "アニメーション", [this, engine](AnimationComponent* animation)
            {
                ImGui::Text("ハンドル : Index(%u), Gen(%u)", animation->handle.index, animation->handle.generation);

                // アニメーションストアを取得
                auto animationStore = engine->GetRenderContext()->GetAnimationStore();
                if (!animationStore)
                {
                    ImGui::Text("アニメーションストアが見つかりません");
                    return;
                }

                // アニメーションデータを取得
                auto animationData = animationStore->GetAnimationData(animation->handle);
                if (!animationStore->GetAnimationData(animation->handle))
                {
                    ImGui::Text("アニメーションデータが見つかりません");
                    return;
                }

                ImGui::DragFloat("再生時間", &animation->currentTime, 0.01f, 0.0f, animationData->duration);
                ImGui::DragFloat("再生速度", &animation->playbackSpeed, 0.01f);

                // 再生・停止ボタンの実装
                if (ImGui::Button("再生", ImVec2(80, 0)))
                {

                }

                ImGui::SameLine();

                if (ImGui::Button("停止", ImVec2(80, 0)))
                {

                }
            });

        // TextureComponent
        DrawComponent<TextureComponent>(entity, "テクスチャ", [this, engine](TextureComponent* texture)
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
            });

        // CubemapComponent
        DrawComponent<CubemapComponent>(entity, "キューブマップ", [this, engine](CubemapComponent* cubemap)
            {
                // 現在設定されているキューブマップのハンドル番号を表示
                ImGui::Text("ハンドル : Index(%u), Gen(%u)", cubemap->handle.index, cubemap->handle.generation);

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
                                    cubemap->handle = droppedHandle;
                            }

                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            });

        // ModelComponent
        DrawComponent<ModelComponent>(entity, "モデル", [this, engine](ModelComponent* model)
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
            });

        // UVTransformComponent
        DrawComponent<UVTransformComponent>(entity, "UVトランスフォーム", [this, engine](UVTransformComponent* uvTransform)
            {
                ImGui::DragFloat2("オフセット##UV", &uvTransform->offset.x, 0.1f);
                ImGui::DragFloat("回転##UV", &uvTransform->radian, 0.01f);
                ImGui::DragFloat2("大きさ##UV", &uvTransform->scale.x, 0.1f);
            });

        // DirectionalLightComponent
        DrawComponent<DirectionalLightComponent>(entity, "平行光源", [this, engine](DirectionalLightComponent* directionalLight)
            {
                ImGui::Checkbox("有効##平行光源", &directionalLight->isEnabled);
                if (ImGui::DragFloat3("方向##平行光源", &directionalLight->direction.x, 0.1f))
                {
                    // 入力された方向ベクトルを正規化する
                    directionalLight->direction = directionalLight->direction.Normalize();
                }
                ImGui::ColorEdit3("色##平行光源", &directionalLight->color.x);
                ImGui::DragFloat("強さ##平行光源", &directionalLight->intensity, 0.1f);
            });

        // PointLightComponent
        DrawComponent<PointLightComponent>(entity, "ポイントライト", [this, engine](PointLightComponent* pointLight)
            {
                ImGui::Checkbox("有効##ポイントライト", &pointLight->isEnabled);
                ImGui::DragFloat3("位置##ポイントライト", &pointLight->position.x, 0.01f);
                ImGui::ColorEdit3("色##ポイントライト", &pointLight->color.x);
                ImGui::DragFloat("強さ##ポイントライト", &pointLight->intensity, 0.1f);
                ImGui::DragFloat("半径##ポイントライト", &pointLight->radius, 0.1f);
                ImGui::DragFloat("減衰##ポイントライト", &pointLight->decay, 0.1f);
            });

        // SpotLightComponent
        DrawComponent<SpotLightComponent>(entity, "スポットライト", [this, engine](SpotLightComponent* spotLight)
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
            });

        // AudioComponent
        DrawComponent<AudioComponent>(entity, "オーディオ", [this, engine, entity](AudioComponent* audio)
            {
                // 現在設定されているオーディオのハンドル番号を表示
                ImGui::Text("ハンドル : Index(%u), Gen(%u)", audio->audioHandle.index, audio->audioHandle.generation);

                // ドロップを受け付けるための専用エリアを作成
                ImGui::Button("オーディオをドロップ", ImVec2(-1, 30));
                if (ImGui::BeginDragDropTarget())
                {
                    // ペイロード名 "AUDIO_HANDLE" を受け取る
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AUDIO_HANDLE"))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(Detail::AudioHandle));
                        Detail::AudioHandle droppedHandle = *(const Detail::AudioHandle*)payload->Data;
                        audio->audioHandle = droppedHandle;
                    }
                    ImGui::EndDragDropTarget();
                }

                // パラメータ設定
                ImGui::DragFloat("音量##オーディオ", &audio->volume, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("ピッチ##オーディオ", &audio->pitch, 0.01f, 0.1f, 4.0f); // ピッチの範囲目安
                ImGui::Checkbox("ループ##オーディオ", &audio->isLoop);

                // 重複再生の許可設定（SE用）
                ImGui::Checkbox("重複再生を許可 (SE用)##オーディオ", &audio->allowOverlap);

                ImGui::Separator();

                // 再生・停止ボタンの実装
                if (ImGui::Button("再生", ImVec2(80, 0)))
                {
                    if (engine)
                    {
                        // エンジンからAudioSystemを取得して再生
                        if (auto audioSystem = engine->GetAudioSystem())
                        {
                            audioSystem->PlayAudio(entity);
                        }
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("停止", ImVec2(80, 0)))
                {
                    if (engine)
                    {
                        // エンジンからAudioSystemを取得して停止
                        if (auto audioSystem = engine->GetAudioSystem())
                        {
                            audioSystem->StopAudio(entity);
                        }
                    }
                }
            });

        // InputComponent
        DrawComponent<InputComponent>(entity, "入力", [this, engine](InputComponent* input)
            {
                // デバイスの切り替え
                const char* deviceNames[] = { "キーボード＆マウス", "ゲームパッド" };
                int currentDevice = static_cast<int>(input->deviceType);
                if (ImGui::Combo("入力デバイス##入力", &currentDevice, deviceNames, IM_ARRAYSIZE(deviceNames)))
                {
                    input->deviceType = static_cast<InputDeviceType>(currentDevice);
                }

                ImGui::Separator();

                // 選択されたデバイスに応じた割り当て設定
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

                // 実際の入力状態の可視化（デバッグ・疑似操作用）
                ImGui::Text("【現在の入力状態（デバッグ用）】");
                ImGui::DragFloat2("移動入力##入力", &input->moveInput.x, 0.01f);
                ImGui::DragFloat2("視点入力##入力", &input->cameraInput.x, 0.01f);
                ImGui::Checkbox("アクション押下##入力", &input->isActionPressed);
                ImGui::Checkbox("アクショントリガー##入力", &input->isActionTriggered);
            });

        // HierarchyComponent
        DrawComponent<HierarchyComponent>(entity, "階層", [this, engine](HierarchyComponent* hierarchy)
            {
                if (!hierarchy->children.empty())
                {
                    ImGui::Separator();
                    ImGui::Text("子エンティティ");

                    // 子エンティティに対して再帰処理を行う
                    for (Entity child : hierarchy->children)
                    {
                        DrawEntityComponents(child);
                    }
                }
            });

        // JointHierarchyComponent
        DrawComponent<JointHierarchyComponent>(entity, "ジョイント階層", [this, engine](JointHierarchyComponent* jointHierarchy)
            {
                if (!jointHierarchy->children.empty())
                {
                    ImGui::Separator();
                    ImGui::Text("子エンティティ (ジョイント)");
                    // 子エンティティに対して再帰処理を行う
                    for (Entity child : jointHierarchy->children)
                    {
                        DrawEntityComponents(child);
                    }
                }
            });


        ImGui::Separator();
        if (ImGui::Button("コンポーネントを追加", ImVec2(-1, 30)))
        {
            ImGui::OpenPopup("コンポーネント");
        }

        if (ImGui::BeginPopup("コンポーネント"))
        {
            if (!registry_->HasComponent<TagComponent>(entity) && ImGui::MenuItem("タグ"))
                registry_->AddComponent<TagComponent>(entity, TagComponent{});

            if (!registry_->HasComponent<TransformComponent>(entity) && ImGui::MenuItem("トランスフォーム"))
                registry_->AddComponent<TransformComponent>(entity, TransformComponent{});

            if (!registry_->HasComponent<CameraComponent>(entity) && ImGui::MenuItem("カメラ"))
                registry_->AddComponent<CameraComponent>(entity, CameraComponent{});

            if (!registry_->HasComponent<PivotPointComponent>(entity) && ImGui::MenuItem("ピボットポイント"))
                registry_->AddComponent<PivotPointComponent>(entity, PivotPointComponent{});

            if (!registry_->HasComponent<MaterialComponent>(entity) && ImGui::MenuItem("マテリアル"))
                registry_->AddComponent<MaterialComponent>(entity, MaterialComponent{});

            if (!registry_->HasComponent<ScriptComponent>(entity) && ImGui::MenuItem("スクリプト"))
                registry_->AddComponent<ScriptComponent>(entity, ScriptComponent{});

            if (!registry_->HasComponent<InputComponent>(entity) && ImGui::MenuItem("入力"))
                registry_->AddComponent<InputComponent>(entity, InputComponent{});

            if (!registry_->HasComponent<AudioComponent>(entity) && ImGui::MenuItem("オーディオ"))
                registry_->AddComponent<AudioComponent>(entity, AudioComponent{});

            ImGui::EndPopup();
        }


        ImGui::TreePop();
    }
}
