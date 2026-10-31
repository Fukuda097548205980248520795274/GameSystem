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

	if (!ImGui::Begin("アセット", nullptr, ImGuiWindowFlags_MenuBar))
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
		if (ImGui::BeginTabItem("テクスチャ"))
		{
			currentCategory_ = AssetCategory::Texture;
			DrawTextureTab(renderContext, textureStore);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("モデル"))
		{
			currentCategory_ = AssetCategory::Model;
			DrawModelTab(renderContext, modelStore);
			ImGui::EndTabItem();
		}


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
			// ディレクトリがドロップされた場合は、ディレクトリごとコピーしてモデルファイルを探す
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
			else if (ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx")
			{
				// モデルの処理

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

		// ドラッグアンドドロップのソースとして設定
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			// ドラッグ中のペイロードとしてテクスチャハンドルを設定
			ImGui::SetDragDropPayload("TEXTURE_HANDLE", &handle, sizeof(TextureHandle));

			ImGui::Text("テクスチャ: %s", fileName.c_str());
			ImGui::EndDragDropSource();
		}

		// ツールチップ表示
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();

			std::string path = textureStore->GetFilePath(handle);
			size_t width = textureStore->GetTextureWidth(handle);
			size_t height = textureStore->GetTextureHeight(handle);
			TextureType type = textureStore->GetType(handle);

			ImGui::Text("ファイルパス : %s", path.c_str());
			ImGui::Text("解像度 : %zu x %zu", width, height);
			ImGui::Text("種類 : %s", type == TextureType::Cubemap ? "Cubemap" : "Texture2D");

			ImGui::Separator();

			SRVDescriptorHandle srvHandle = textureStore->GetSrvHandle(handle);

			float previewWidth = std::min(256.0f, static_cast<float>(width));
			float previewHeight = previewWidth * (static_cast<float>(height) / static_cast<float>(width));

			if (type == TextureType::Texture2D)
			{
				ImGui::Image(static_cast<ImTextureID>(srvHandle.gpuHandle.ptr), ImVec2(previewWidth, previewHeight));
			}
			else if (type == TextureType::Cubemap)
			{
				ImGui::TextDisabled("キューブマップのプレビューは非対応です");
			}

			ImGui::EndTooltip();
		}
	}
	ImGui::EndChild();


	// 選択されたテクスチャが有効な場合に削除ボタンを表示
	if (selectedTextureHandle_.IsValid())
	{
		if (ImGui::Button("選択したテクスチャを削除", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			auto engine = Engine::GetInstance();

			try
			{
				std::string path = textureStore->GetFilePath(selectedTextureHandle_);
				if (std::filesystem::exists(path))
					std::filesystem::remove(path);

				// テクスチャストアから削除
				textureStore->Remove(selectedTextureHandle_, renderContext->GetHeap(), engine->GetFrameIndex());

				// 選択状態をリセット
				selectedTextureHandle_ = TextureHandle::Invalid();
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

		// ファイル名を取得
		std::string filePath = modelData->filePath;
		size_t slashPos = filePath.find_last_of("/\\");
		std::string fileName = (slashPos != std::string::npos) ? filePath.substr(slashPos + 1) : filePath;

		// 選択状態の判定
		bool isSelected = (selectedModelHandle_ == handle);
		if (ImGui::Selectable((fileName + "##" + std::to_string(i)).c_str(), isSelected))
		{
			selectedModelHandle_ = handle;
		}

		// ドラッグアンドドロップのソースとして設定
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			// ドラッグ中のペイロードとしてモデルハンドルを設定
			ImGui::SetDragDropPayload("MODEL_HANDLE", &handle, sizeof(ModelHandle));

			ImGui::Text("モデル: %s", fileName.c_str());
			ImGui::EndDragDropSource();
		}

		// ツールチップ表示
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();

			ImGui::Text("ファイルパス : %s", filePath.c_str());
			ImGui::Text("メッシュ数 : %zu", modelData->meshes.size());
			ImGui::Text("ノード数 : %zu", modelData->nodes.size());

			ImGui::EndTooltip();
		}
	}
	ImGui::EndChild();

	// 選択されたモデルが有効な場合に削除ボタンを表示
	if (selectedModelHandle_.IsValid())
	{
		if (ImGui::Button("選択したモデルを削除", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			auto engine = Engine::GetInstance();

			try
			{
				ModelData* data = modelStore->GetModelData(selectedModelHandle_);
				if (data)
				{
					std::string path = data->filePath;
					if (std::filesystem::exists(path))
						std::filesystem::remove(path);
				}

				// モデルストアから削除
				modelStore->Unload(selectedModelHandle_, engine->GetFrameIndex());

				// 選択状態をリセット
				selectedModelHandle_ = ModelHandle::Invalid();
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				if (engine) engine->Log(LogLevel::Error, "モデルファイルの削除に失敗しました : " + std::string(e.what()));
			}
		}
	}
	else
	{
		// 未選択時のプレースホルダー
		ImGui::TextDisabled("モデルを選択すると削除ボタンが表示されます");
	}
}