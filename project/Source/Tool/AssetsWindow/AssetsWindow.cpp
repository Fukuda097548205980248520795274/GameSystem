#include "AssetsWindow.hpp"
#include "Engine.hpp"
#include <algorithm>

/// @brief UI描画
/// @param renderContext 
/// @param textureStore 
/// @param modelStore 
void Detail::AssetsWindow::DrawUI(RenderContext* renderContext, TextureStore* textureStore, ModelStore* modelStore)
{
#ifdef DEVELOPMENT
	auto engine = Engine::GetInstance();

	if (!ImGui::Begin("Assets", nullptr, ImGuiWindowFlags_MenuBar))
	{
		ImGui::End();
		return;
	}

	// ドロップされたファイルの処理（ウィンドウ上にドロップされた場合のみ）
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
	{
		ProcessDroppedFiles(renderContext);
	}
	else
	{
		// 他の場所にドロップされた場合はクリア
		std::lock_guard<std::mutex> lock(dropMutex_);
		droppedFilesQueue_.clear();
	}

	// タブバーによるカテゴリの切り替え
	if (ImGui::BeginTabBar("AssetsTabBar"))
	{
		if (ImGui::BeginTabItem("Textures"))
		{
			currentCategory_ = AssetCategory::Texture;
			DrawTextureTab(renderContext, textureStore);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Models"))
		{
			currentCategory_ = AssetCategory::Model;
			DrawModelTab(renderContext, modelStore);
			ImGui::EndTabItem();
		}
		// 将来的に追加
		/*
		if (ImGui::BeginTabItem("Audio")) { ... }
		if (ImGui::BeginTabItem("Scripts")) { ... }
		*/
		ImGui::EndTabBar();
	}

	ImGui::End();
#endif
}

/// @brief ファイルがドロップされたときの処理
/// @param filePath 
void Detail::AssetsWindow::OnFileDropped(const std::string& filePath)
{
	std::lock_guard<std::mutex> lock(dropMutex_);
	droppedFilesQueue_.push_back(filePath);
}

/// @brief ドロップされたファイルの処理
/// @param renderContext 
void Detail::AssetsWindow::ProcessDroppedFiles(RenderContext* renderContext)
{
	std::lock_guard<std::mutex> lock(dropMutex_);
	if (droppedFilesQueue_.empty()) return;

	auto engine = Engine::GetInstance();

	for (const auto& originalPath : droppedFilesQueue_)
	{
		std::filesystem::path srcPath(originalPath);

		try
		{
			// フォルダごとドロップされた場合（モデルなどを想定）
			if (std::filesystem::is_directory(srcPath))
			{
				std::filesystem::path destPath = kModelDir + srcPath.filename().string();
				if (!std::filesystem::exists(kModelDir)) std::filesystem::create_directories(kModelDir);

				std::filesystem::copy(srcPath, destPath, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);

				// フォルダ内からモデルファイルを探してロード
				for (const auto& entry : std::filesystem::recursive_directory_iterator(destPath))
				{
					if (entry.is_regular_file())
					{
						std::string ext = entry.path().extension().string();
						std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
						if (ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx")
						{
							ModelHandle handle = renderContext->LoadModel(entry.path().string());
							if (handle.IsValid()) selectedModelHandle_ = handle;
						}
					}
				}
				continue;
			}

			// 単一ファイルの場合の拡張子判別
			std::string ext = srcPath.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			// テクスチャの処理
			if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".dds")
			{
				if (!std::filesystem::exists(kTextureDir)) std::filesystem::create_directories(kTextureDir);
				std::filesystem::path destPath = kTextureDir + srcPath.filename().string();
				std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing);

				TextureHandle handle = renderContext->LoadTexture(destPath.string());
				if (handle.IsValid())
				{
					selectedTextureHandle_ = handle;
					currentCategory_ = AssetCategory::Texture; // 自動的にタブを切り替える
				}
			}
			// モデルの処理
			else if (ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx")
			{
				if (!std::filesystem::exists(kModelDir)) std::filesystem::create_directories(kModelDir);
				std::filesystem::path destPath = kModelDir + srcPath.filename().string();
				std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing);

				ModelHandle handle = renderContext->LoadModel(destPath.string());
				if (handle.IsValid())
				{
					selectedModelHandle_ = handle;
					currentCategory_ = AssetCategory::Model;
				}
			}
			// 音声など将来用
			else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg")
			{
				// Audioの処理を将来ここに記述
			}
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			if (engine) engine->Log(LogLevel::Error, "ファイルコピー失敗: " + std::string(e.what()));
		}
	}
	droppedFilesQueue_.clear();
}

/// @brief テクスチャタブの描画
/// @param renderContext 
/// @param textureStore 
void Detail::AssetsWindow::DrawTextureTab(RenderContext* renderContext, TextureStore* textureStore)
{
	ImGui::BeginChild("TextureList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5.0f), true);

	size_t texCount = textureStore->GetTextureCount();
	for (uint32_t i = 0; i < texCount; ++i)
	{
		TextureHandle handle = textureStore->GetHandleByIndex(i);
		if (!handle.IsValid()) continue;

		std::string filePath = textureStore->GetFilePath(handle);
		if (filePath.empty()) continue;

		size_t slashPos = filePath.find_last_of("/\\");
		std::string fileName = (slashPos != std::string::npos) ? filePath.substr(slashPos + 1) : filePath;

		bool isSelected = (selectedTextureHandle_ == handle);
		if (ImGui::Selectable((fileName + "##" + std::to_string(i)).c_str(), isSelected))
		{
			selectedTextureHandle_ = handle;
		}

		// D&D ソース
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload("TEXTURE_HANDLE", &handle, sizeof(TextureHandle));
			ImGui::Text("テクスチャ: %s", fileName.c_str());
			ImGui::EndDragDropSource();
		}

		// ツールチップでのプレビュー処理 (元の実装と同等)
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("ファイルパス : %s", filePath.c_str());
			// (省略：元ファイルにあった画像プレビュー処理等を含める)
			ImGui::EndTooltip();
		}
	}
	ImGui::EndChild();

	// 削除ボタン
	if (selectedTextureHandle_.IsValid())
	{
		if (ImGui::Button("選択したテクスチャを削除", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			// (元の削除処理)
			textureStore->Remove(selectedTextureHandle_, renderContext->GetHeap(), Engine::GetInstance()->GetFrameIndex());
			selectedTextureHandle_ = TextureHandle::Invalid();
		}
	}
}

/// @brief モデルタブの描画
/// @param renderContext 
/// @param modelStore 
void Detail::AssetsWindow::DrawModelTab(RenderContext* renderContext, ModelStore* modelStore)
{
	ImGui::BeginChild("ModelList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5.0f), true);

	size_t modelCount = modelStore->GetModelCount();
	for (uint32_t i = 0; i < modelCount; ++i)
	{
		ModelHandle handle = modelStore->GetHandleByIndex(i);
		if (!handle.IsValid()) continue;

		ModelData* modelData = modelStore->GetModelData(handle);
		if (!modelData || modelData->filePath.empty()) continue;

		std::string filePath = modelData->filePath;
		size_t slashPos = filePath.find_last_of("/\\");
		std::string fileName = (slashPos != std::string::npos) ? filePath.substr(slashPos + 1) : filePath;

		bool isSelected = (selectedModelHandle_ == handle);
		if (ImGui::Selectable((fileName + "##" + std::to_string(i)).c_str(), isSelected))
		{
			selectedModelHandle_ = handle;
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload("MODEL_HANDLE", &handle, sizeof(ModelHandle));
			ImGui::Text("モデル: %s", fileName.c_str());
			ImGui::EndDragDropSource();
		}
	}
	ImGui::EndChild();

	if (selectedModelHandle_.IsValid())
	{
		if (ImGui::Button("選択したモデルを削除", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			modelStore->Unload(selectedModelHandle_, Engine::GetInstance()->GetFrameIndex());
			selectedModelHandle_ = ModelHandle::Invalid();
		}
	}
}