#include "SceneEditor.hpp"
#include "../SceneSerializer/SceneSerializer.hpp"
#include "Engine.hpp"

/// @brief エディタの初期化
void Detail::SceneEditor::Initialize()
{
    sceneManager_ = std::make_unique<SceneManager>();

    // 起動時にプロジェクトのシーンリストとスタートアップシーン名を復元する
    Detail::DeserializeSceneList(sceneManager_.get(), kProjectSettingsPath, kProjectSettingsBinPath);

    // スタートアップシーンが設定されていれば、初期シーンとして読み込みを予約する
    const std::string& startupScene = sceneManager_->GetStartupSceneName();
    if (!startupScene.empty())
    {
        sceneManager_->Initialize(startupScene);
        Engine::GetInstance()->Log(LogLevel::Info, "スタートアップシーンを読み込みます : " + startupScene);
    }
    else
    {
        Engine::GetInstance()->Log(LogLevel::Warning, "スタートアップシーンが設定されていません。");
    }
}

/// @brief 新規シーンを作成する
void Detail::SceneEditor::CreateNewScene(const std::string& sceneName)
{
    if (sceneName.empty()) return;

    auto engine = Engine::GetInstance();
    auto registry = engine->GetRegistryECS();

    // 現在のエンティティをすべて破棄して画面をまっさらにする
    registry->Clear();

    // SceneManagerに新しいシーンとして登録
    std::string filePath = "./Assets/Data/Scenes/" + sceneName + ".json";
    sceneManager_->AddScene(sceneName, filePath);

    // 現在のシーン名を新しいものに切り替える
    sceneManager_->SetCurrentSceneName(sceneName);

    // 生成したデフォルト状態をECSシリアライザで即座に保存 (JSON & BIN)
    sceneManager_->SaveCurrentScene();

    // プロジェクト全体のシーンリストも更新して保存
    Detail::SerializeSceneList(sceneManager_.get(), kProjectSettingsPath, kProjectSettingsBinPath);

    // ログ出力
    engine->Log(LogLevel::Info, "新たなシーンが作成されました : " + sceneName);
}

/// @brief シーンを削除する
/// @param sceneName 削除するシーンの名前
void Detail::SceneEditor::DeleteScene(const std::string& sceneName)
{
    // スタートアップシーンに設定されている場合は解除する
    if (sceneManager_->GetStartupSceneName() == sceneName)
    {
        sceneManager_->SetStartupSceneName("");
    }

    // 現在開いているシーンを削除した場合は表示名をクリア
    if (sceneManager_->GetCurrentSceneName() == sceneName)
    {
        sceneManager_->SetCurrentSceneName("");
    }

    // シーンマネージャーのリストから除外
    sceneManager_->RemoveScene(sceneName);

    // プロジェクト設定を保存して変更を永続化
    Detail::SerializeSceneList(sceneManager_.get(), kProjectSettingsPath, kProjectSettingsBinPath);

    Engine::GetInstance()->Log(LogLevel::Info, "シーンをプロジェクトから削除しました : " + sceneName);
}

/// @brief 現在のシーンを保存する
void Detail::SceneEditor::SaveCurrentScene()
{
    sceneManager_->SaveCurrentScene();
    Engine::GetInstance()->Log(LogLevel::Info, "シーンを保存しました : " + sceneManager_->GetCurrentSceneName());
}

/// @brief シーンを読み込む
void Detail::SceneEditor::LoadScene(const std::string& sceneName)
{
    // 次のフレームで安全に読み込みが行われるよう予約
    sceneManager_->LoadScene(sceneName);
}

/// @brief エディタのUIを描画する
void Detail::SceneEditor::DrawUI()
{
#ifdef DEVELOPMENT

    ImGui::Begin("シーンエディタ");

    // 現在のステータス表示
    ImGui::Text("現在のシーン :");
    ImGui::SameLine();
    std::string currentScene = sceneManager_->GetCurrentSceneName();
    if (currentScene.empty())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "未選択");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", currentScene.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // クイックアクション（新規作成・保存ボタンを横並びに配置）
    if (ImGui::Button("新しいシーンを作成", ImVec2(150, 30)))
    {
        ImGui::OpenPopup("シーン名を設定");
    }
    ImGui::SameLine();
    if (ImGui::Button("現在のシーンを保存", ImVec2(150, 30)))
    {
        SaveCurrentScene();
    }

    // 新規シーン作成のモーダルポップアップ
    if (ImGui::BeginPopupModal("シーン名を設定", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char newSceneName[128] = "NewScene";
        ImGui::InputText("シーン名", newSceneName, IM_ARRAYSIZE(newSceneName));
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("作成", ImVec2(120, 0)))
        {
            CreateNewScene(std::string(newSceneName));
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // プロジェクト設定（折りたたみ可能にしてスペースを節約）
    if (ImGui::CollapsingHeader("プロジェクト設定", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Spacing();
        std::string currentStartup = sceneManager_->GetStartupSceneName();
        ImGui::Text("開始シーン :");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::BeginCombo("##StartupSceneCombo", currentStartup.empty() ? "(未設定)" : currentStartup.c_str()))
        {
            for (const auto& scene : sceneManager_->GetSceneList())
            {
                bool isSelected = (currentStartup == scene.name);
                if (ImGui::Selectable(scene.name.c_str(), isSelected))
                {
                    sceneManager_->SetStartupSceneName(scene.name);
                    Detail::SerializeSceneList(sceneManager_.get(), kProjectSettingsPath, kProjectSettingsBinPath);
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Spacing();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // シーンブラウザ
    ImGui::Text("プロジェクト内のシーン");
    ImGui::Spacing();

    std::string sceneToDelete = "";
    static std::string sceneToOpenDeleteModal = "";

    // シーンリストの表示領域を確保し、スクロール可能にする
    float availableHeight = ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y;
    float childHeight = std::max(availableHeight, 200.0f);

    if (ImGui::BeginChild("SceneListRegion", ImVec2(0, childHeight), true))
    {
        float buttonWidth = 140.0f;
        float buttonHeight = 60.0f;
        float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        ImGuiStyle& style = ImGui::GetStyle();

        const auto& sceneList = sceneManager_->GetSceneList();
        int sceneCount = static_cast<int>(sceneList.size());
        int currentIdx = 0;

        for (const auto& scene : sceneList)
        {
            ImGui::PushID(scene.name.c_str());

            if (ImGui::Button(scene.name.c_str(), ImVec2(buttonWidth, buttonHeight)))
            {
                LoadScene(scene.name);
            }

            // 右クリックメニュー
            if (ImGui::BeginPopupContextItem("SceneContextMenu"))
            {
                if (ImGui::MenuItem("削除"))
                {
                    sceneToOpenDeleteModal = scene.name;
                }
                ImGui::EndPopup();
            }

            if (sceneToOpenDeleteModal == scene.name)
            {
                ImGui::OpenPopup("削除確認");
                sceneToOpenDeleteModal = "";
            }

            // 削除確認モーダル
            if (ImGui::BeginPopupModal("削除確認", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("シーン '%s' をプロジェクトから削除しますか？", scene.name.c_str());
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("削除", ImVec2(140, 0)))
                {
                    sceneToDelete = scene.name;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("キャンセル", ImVec2(100, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            // 折り返し判定
            float lastButtonX2 = ImGui::GetItemRectMax().x;
            float nextButtonX2 = lastButtonX2 + style.ItemSpacing.x + buttonWidth;
            currentIdx++;

            if (currentIdx < sceneCount && nextButtonX2 < windowVisibleX2)
            {
                ImGui::SameLine();
            }

            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    // 削除処理の実行
    if (!sceneToDelete.empty())
    {
        DeleteScene(sceneToDelete);
    }

    ImGui::End();

#endif
}
