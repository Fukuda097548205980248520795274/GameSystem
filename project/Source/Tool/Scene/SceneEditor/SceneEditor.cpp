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
        sceneManager_->LoadScene(startupScene);
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
    ImGui::Begin("シーンエディタ");

    // 現在のシーン名を表示
    ImGui::Text("現在のシーン : %s", sceneManager_->GetCurrentSceneName().c_str());
    ImGui::Separator();


    ImGui::Text("プロジェクト設定 :");

    // 開始シーンを選択するプルダウン
    std::string currentStartup = sceneManager_->GetStartupSceneName();
    if (ImGui::BeginCombo("開始シーン", currentStartup.c_str()))
    {
        for (const auto& scene : sceneManager_->GetSceneList())
        {
            bool isSelected = (currentStartup == scene.name);
            if (ImGui::Selectable(scene.name.c_str(), isSelected))
            {
                // 選択されたシーンを開始シーンに設定し、プロジェクト設定を保存する
                sceneManager_->SetStartupSceneName(scene.name);
                Detail::SerializeSceneList(sceneManager_.get(), kProjectSettingsPath, kProjectSettingsBinPath);
            }

            // プルダウンを開いた際に、現在設定されているアイテムにフォーカスを合わせる
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();


    // 新規シーン作成UI
    static char newSceneName[128] = "NewScene";
    ImGui::InputText("シーン名", newSceneName, IM_ARRAYSIZE(newSceneName));

    if (ImGui::Button("シーン作成"))
    {
        CreateNewScene(std::string(newSceneName));
    }

    ImGui::Separator();

    // 保存UI
    if (ImGui::Button("シーンを保存", ImVec2(-1.0f, 30.0f)))
    {
        SaveCurrentScene();
    }

    ImGui::Separator();
    ImGui::Text("プロジェクト内のシーン :");

    // シーンリストを表示し、ボタンで読み込めるようにする
    for (const auto& scene : sceneManager_->GetSceneList())
    {
        if (ImGui::Button(("読み込み :  " + scene.name).c_str(), ImVec2(-1.0f, 0.0f)))
        {
            LoadScene(scene.name);
        }
    }

    ImGui::End();
}
