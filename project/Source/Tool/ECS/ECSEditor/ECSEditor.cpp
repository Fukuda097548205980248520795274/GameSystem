#include "ECSEditor.hpp"
#include "Engine.hpp"

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
	// エンジンのインスタンスを取得
    registry_ = Engine::GetInstance()->GetRegistryECS();

#ifdef DEVELOPMENT

	// UIウィンドウの描画
    DrawPlayControls();
	DrawHierarchy();
	DrawInspector();

#endif
}

/// @brief プレイコントロールの描画
void Detail::ECSEditor::DrawPlayControls()
{
    auto engine = Engine::GetInstance();
    PlayState currentState = engine->GetPlayState();

    // ツールバー風にするため、タイトルバーやスクロールバーなどの装飾を消す
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

    // 再生中はツールバー全体の背景色を少し暗くする（UnityのPlaymode tint風）
    if (currentState == PlayState::Playing)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    }

    ImGui::Begin("Play Controls", nullptr, windowFlags);

    // ボタンの固定サイズ（ここで指定したサイズ以上に大きくならないようにする）
    float buttonWidth = 60.0f;
    float buttonHeight = 24.0f;

    // 現在のカーソル位置（左端）を取得
    float cursorPosX = ImGui::GetCursorPosX();

    // 描画可能な領域の幅を取得
    float availWidth = ImGui::GetContentRegionAvail().x;

    // 中央揃えのためのX座標を計算してカーソルを移動
    ImGui::SetCursorPosX(cursorPosX + (availWidth - buttonWidth) * 0.5f);

    if (currentState == PlayState::Editor)
    {
        // 編集モード：Playボタン（例えば少し緑がかった色にする）
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));

        if (ImGui::Button("Play", ImVec2(buttonWidth, buttonHeight)))
        {
            // 現在の編集状態をバックアップ
            backupRegistry_ = registry_->Clone();
            // 再生モードへ移行
            engine->SetPlayState(PlayState::Playing);
        }
        ImGui::PopStyleColor(3);
    }
    else if (currentState == PlayState::Playing)
    {
        // 再生モード：Stopボタン（赤や目立つ色にして停止を促す）
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

        if (ImGui::Button("Stop", ImVec2(buttonWidth, buttonHeight)))
        {
            // エディタモードへ戻す
            engine->SetPlayState(PlayState::Editor);

            if (backupRegistry_)
            {
                // 安全な差し替えを予約する
                engine->QueueRestoreRegistry(std::move(backupRegistry_));
                backupRegistry_ = nullptr;

                // 選択状態をリセット
                selectedEntity_ = kNullEntity;
            }
        }
        ImGui::PopStyleColor(3);
    }



    ImGui::SameLine();

    // Saveボタンの追加
    if (ImGui::Button("Save Scene", ImVec2(80.0f, buttonHeight)))
    {
        if (engine->SaveScene("MainScene"))
        {
            engine->Log(LogLevel::Info, "ECSデータを保存しました。");
        }
        else
        {
            engine->Log(LogLevel::Error, "ECSデータの保存に失敗しました。");
        }
    }



    ImGui::End();

    // Pushした背景色のPop
    if (currentState == PlayState::Playing)
    {
        ImGui::PopStyleColor();
    }
}