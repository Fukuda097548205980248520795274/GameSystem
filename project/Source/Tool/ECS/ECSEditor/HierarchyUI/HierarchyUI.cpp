#include "../ECSEditor.hpp"
#include "Engine.hpp"

//// @brief ヒエラルキーUIを描画する
void Detail::ECSEditor::DrawHierarchy()
{
#ifdef DEVELOPMENT
    auto engine = Engine::GetInstance();
    ImGui::Begin("ヒエラルキー");


    // 「+ 作成」ドロップダウンメニュー（エンティティの塊を配置）
    if (ImGui::Button("+ 作成"))
    {
        ImGui::OpenPopup("CreateEntityMenu");
    }

    // 作成メニューの中身をラムダ化（ボタンと空きスペース右クリックで共用）
    auto DrawCreateMenuItems = [&]()
        {
            if (ImGui::MenuItem("空のEntity"))
            {
                selectedEntity_ = registry_->CreateEntity();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("レンダーパス"))
            {
                selectedEntity_ = engine->CreatePass(0, BlendMode::None);
            }

            if (ImGui::MenuItem("カメラ"))
            {
                selectedEntity_ = engine->CreateCameraEntity();
            }
            if (ImGui::MenuItem("スカイボックス"))
            {
                selectedEntity_ = engine->CreateSkyboxEntity();
            }

            if (ImGui::BeginMenu("ライト"))
            {
                if (ImGui::MenuItem("平行光源 (Directional)"))
                {
                    selectedEntity_ = engine->CreateDirectionalLightEntity(
                        { 0.0f, 10.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, 1.0f, true);
                }
                if (ImGui::MenuItem("ポイントライト (Point)"))
                {
                    selectedEntity_ = engine->CreatePointLightEntity(
                        { 0.0f, 2.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, 1.0f, 10.0f, 1.0f);
                }
                if (ImGui::MenuItem("スポットライト (Spot)"))
                {
                    selectedEntity_ = engine->CreateSpotLightEntity(
                        { 0.0f, 5.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, 1.0f, 10.0f, 1.0f, 0.7f, 0.9f);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("モデル (アセットから配置)"))
            {
                auto modelStore = engine->GetRenderContext()->GetModelStore();
                size_t modelCount = modelStore->GetModelCount();
                if (modelCount == 0)
                {
                    ImGui::TextDisabled("読み込み済みのモデルがありません");
                }
                for (uint32_t i = 0; i < modelCount; ++i)
                {
                    ModelHandle handle = modelStore->GetHandleByIndex(i);
                    if (!handle.IsValid()) continue;

                    ModelData* data = modelStore->GetModelData(handle);
                    if (!data || data->filePath.empty()) continue;

                    size_t slashPos = data->filePath.find_last_of("/\\");
                    std::string fileName = (slashPos != std::string::npos) ? data->filePath.substr(slashPos + 1) : data->filePath;

                    if (ImGui::MenuItem((fileName + "##" + std::to_string(i)).c_str()))
                    {
                        selectedEntity_ = engine->CreateModelEntity(handle);
                    }
                }
                ImGui::EndMenu();
            }
        };

    if (ImGui::BeginPopup("CreateEntityMenu"))
    {
        DrawCreateMenuItems();
        ImGui::EndPopup();
    }

    ImGui::Separator();



    // エンティティ一覧（BeginChild で囲んで全体ドロップ領域を作る）
    ImGui::BeginChild("HierarchyDropArea", ImVec2(0, 0), false);

    const auto& entities = registry_->GetActiveEntities();
    for (Entity entity : entities)
    {
        bool isChild = false;
        if (auto hierarchyComp = registry_->GetComponent<HierarchyComponent>(entity))
        {
            isChild = hierarchyComp->parent != kNullEntity;
        }
        if (auto jointHierarchyComp = registry_->GetComponent<JointHierarchyComponent>(entity))
        {
            isChild = jointHierarchyComp->parent != kNullEntity;
        }
        if (isChild) continue;

        std::string label = GetEntityDisplayName(entity);
        bool isSelected = (selectedEntity_ == entity);

        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            selectedEntity_ = entity;
        }

        // ドラッグソース（Entity ID をインスペクター等へ渡す）
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("ENTITY_ID", &entity, sizeof(Entity));
            ImGui::Text("%s をドラッグ中", label.c_str());
            ImGui::EndDragDropSource();
        }

        // 既存エンティティへのドロップ受け入れ（スクリプトを直接アタッチ）
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCRIPT_HANDLE"))
            {
                IM_ASSERT(payload->DataSize == sizeof(Detail::ScriptHandle));
                Detail::ScriptHandle droppedHandle = *(const Detail::ScriptHandle*)payload->Data;
                AssignScriptToEntity(entity, droppedHandle);
                selectedEntity_ = entity;
            }
            ImGui::EndDragDropTarget();
        }

        // エンティティ右クリックメニュー
        if (ImGui::BeginPopupContextItem())
        {
            // インスタンス数コンポーネント等を持つプレハブなら複製可能にする
            if (registry_->HasComponent<InstanceCountComponent>(entity))
            {
                if (ImGui::MenuItem("プレハブをインスタンス化"))
                {
                    selectedEntity_ = engine->InstantiatePrefab(entity);
                }
                ImGui::Separator();
            }

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

    // 空きスペースの右クリックメニュー
    if (ImGui::BeginPopupContextWindow("HierarchyWindowContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        DrawCreateMenuItems();
        ImGui::EndPopup();
    }

    ImGui::EndChild();


    // ヒエラルキー全体（空きスペース）へのドロップ受け入れ
    if (ImGui::BeginDragDropTarget())
    {
        // モデルをドロップ → モデルエンティティの塊を生成
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODEL_HANDLE"))
        {
            IM_ASSERT(payload->DataSize == sizeof(Detail::ModelHandle));
            Detail::ModelHandle droppedModel = *(const Detail::ModelHandle*)payload->Data;

            selectedEntity_ = engine->CreateModelEntity(droppedModel);
        }

        // スクリプトを空きスペースにドロップ → 新規エンティティを作成してアタッチ
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCRIPT_HANDLE"))
        {
            IM_ASSERT(payload->DataSize == sizeof(Detail::ScriptHandle));
            Detail::ScriptHandle droppedScript = *(const Detail::ScriptHandle*)payload->Data;

            Entity newEntity = registry_->CreateEntity();
            registry_->AddComponent<TransformComponent>(newEntity, TransformComponent{});
            AssignScriptToEntity(newEntity, droppedScript);
            selectedEntity_ = newEntity;
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::End();
#endif
}
