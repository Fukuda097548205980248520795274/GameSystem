#include "TextureEditor.h"
#include "Engine.h"
#include <filesystem>

void Detail::TextureEditor::DrawUI(RenderContext* renderContext, TextureStore* textureStore)
{
#ifdef DEVELOPMENT

	// Engineのインスタンスを取得
	auto engine = Engine::GetInstance();

	// ドロップされたファイルの処理
    {
        std::lock_guard<std::mutex> lock(dropMutex_);
        for (const auto& originalPath : droppedFilesQueue_)
        {
            std::filesystem::path srcPath(originalPath);

            // コピー先ディレクトリが存在しなければ作成
            if (!std::filesystem::exists(kDir))
                std::filesystem::create_directories(kDir);

            // コピー先のパスを構築
            std::filesystem::path destPath = kDir + srcPath.filename().string();

            try
            {
                // ファイルをプロジェクト内にコピー（既に存在する場合は上書き）
                std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing);

                // コピーした新しいパスでテクスチャをロード
                uint32_t handle = renderContext->LoadTexture(destPath.string());
                if (handle != kInvalidTextureHandle)
                {
                    selectedHandle_ = handle; // 読み込んだものを選択状態に
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                // エラーログ
				if (engine)engine->Log(LogLevel::Error, "テクスチャファイルのコピーに失敗しました : " + std::string(e.what()));
            }
        }
        droppedFilesQueue_.clear(); // 処理が終わったらクリア
    }


    if (!ImGui::Begin("テクスチャエディタ"))
    {
        ImGui::End();
        return;
    }

    // 読み込み済みテクスチャの一覧
    ImGui::Text("ロード済み テクスチャ");
    ImGui::BeginChild("テクスチャ一覧", ImVec2(200, 0), true);

    size_t texCount = textureStore->GetTextureCount();
    for (uint32_t i = 0; i < texCount; ++i)
    {
        std::string filePath = textureStore->GetFilePath(i);
        // ファイルパスが空（無効なデータ）の場合はスキップ
        if (filePath.empty()) continue;

        // ファイル名だけを抽出して表示（見た目をスッキリさせるため）
        size_t slashPos = filePath.find_last_of("/\\");
        std::string fileName = (slashPos != std::string::npos) ? filePath.substr(slashPos + 1) : filePath;

        bool isSelected = (selectedHandle_ == i);
        if (ImGui::Selectable((fileName + "##" + std::to_string(i)).c_str(), isSelected))
        {
            selectedHandle_ = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // 選択中テクスチャの詳細とプレビュー
    ImGui::BeginChild("詳細", ImVec2(0, 0), true);
    if (selectedHandle_ != UINT32_MAX && selectedHandle_ < texCount)
    {
        std::string path = textureStore->GetFilePath(selectedHandle_);
        size_t width = textureStore->GetTextureWidth(selectedHandle_);
        size_t height = textureStore->GetTextureHeight(selectedHandle_);
        TextureType type = textureStore->GetType(selectedHandle_);

        ImGui::Text("ファイルパス : %s", path.c_str());
        ImGui::Text("解像度 : %zu x %zu", width, height);
        ImGui::Text("種類 : %s", type == TextureType::Cubemap ? "Cubemap" : "Texture2D");

        ImGui::Separator();

        // プレビューの描画 (ImGui::ImageにGPUハンドルのポインタを渡す)
        SRVDescriptorHandle srvHandle = textureStore->GetSrvHandle(selectedHandle_);

        // アスペクト比を維持しつつ最大幅を制限して表示
        float availWidth = ImGui::GetContentRegionAvail().x;
        float previewWidth = std::min(availWidth, static_cast<float>(width));
        float previewHeight = previewWidth * (static_cast<float>(height) / static_cast<float>(width));

        ImGui::Image(static_cast<ImTextureID>(srvHandle.gpuHandle.ptr), ImVec2(previewWidth, previewHeight));


        ImGui::Separator();

        // 削除ボタン
        if (ImGui::Button("テクスチャを削除"))
        {
            try
            {
                // ファイルシステム（フォルダ内）から画像を削除
                if (std::filesystem::exists(path))
                    std::filesystem::remove(path);

                // TextureStore側のメモリ・登録データから削除
                textureStore->Remove(selectedHandle_);

                // 選択状態を解除
                selectedHandle_ = UINT32_MAX;
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                if (engine) engine->Log(LogLevel::Error, "テクスチャファイルの削除に失敗しました : " + std::string(e.what()));
            }
        }
    } 
    else
    {
        ImGui::Text("選択しているテクスチャがありません。");
    }
    ImGui::EndChild();

    ImGui::End();

#endif
}

/// @brief ドロップされたファイルを処理する
/// @param filePath 
void Detail::TextureEditor::OnFileDropped(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(dropMutex_);
    droppedFilesQueue_.push_back(filePath);
}