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

	ImGui::Text("ロード済み テクスチャ");

	// 下部に削除ボタンを置くため、少し余白を残してリストのサイズを自動調整
	ImGui::BeginChild("テクスチャ一覧", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5.0f), true);

	size_t texCount = textureStore->GetTextureCount();
	for (uint32_t i = 0; i < texCount; ++i)
	{
		std::string filePath = textureStore->GetFilePath(i);
		if (filePath.empty()) continue;

		size_t slashPos = filePath.find_last_of("/\\");
		std::string fileName = (slashPos != std::string::npos) ? filePath.substr(slashPos + 1) : filePath;

		bool isSelected = (selectedHandle_ == i);
		if (ImGui::Selectable((fileName + "##" + std::to_string(i)).c_str(), isSelected))
		{
			selectedHandle_ = i;
		}

		
		// ツールチップ表示
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();

			std::string path = textureStore->GetFilePath(i);
			size_t width = textureStore->GetTextureWidth(i);
			size_t height = textureStore->GetTextureHeight(i);
			TextureType type = textureStore->GetType(i);

			ImGui::Text("ファイルパス : %s", path.c_str());
			ImGui::Text("解像度 : %zu x %zu", width, height);
			ImGui::Text("種類 : %s", type == TextureType::Cubemap ? "Cubemap" : "Texture2D");

			ImGui::Separator();

			SRVDescriptorHandle srvHandle = textureStore->GetSrvHandle(i);

			// ツールチップ内のプレビュー画像は大きくなりすぎないようにサイズ制限 (最大幅 256px)
			float previewWidth = std::min(256.0f, static_cast<float>(width));
			float previewHeight = previewWidth * (static_cast<float>(height) / static_cast<float>(width));

			ImGui::Image(static_cast<ImTextureID>(srvHandle.gpuHandle.ptr), ImVec2(previewWidth, previewHeight));

			ImGui::EndTooltip();
		}
	}
	ImGui::EndChild();

	
	// 選択されたテクスチャが有効な場合に削除ボタンを表示
	if (selectedHandle_ != UINT32_MAX && selectedHandle_ < texCount)
	{
		if (ImGui::Button("選択したテクスチャを削除", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			try
			{
				std::string path = textureStore->GetFilePath(selectedHandle_);
				if (std::filesystem::exists(path))
					std::filesystem::remove(path);

				// テクスチャストアからも削除
				textureStore->Remove(selectedHandle_, renderContext->GetCore()->GetDevice(), renderContext->GetHeap());

				// 選択状態をリセット
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
		// 未選択時のプレースホルダー
		ImGui::TextDisabled("テクスチャを選択すると削除ボタンが表示されます");
	}

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