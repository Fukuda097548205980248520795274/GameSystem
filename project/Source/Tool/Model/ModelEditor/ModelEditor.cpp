#include "ModelEditor.hpp"
#include "Engine.hpp"
#include <filesystem>

void Detail::ModelEditor::DrawUI(RenderContext* renderContext, ModelStore* modelStore)
{
#ifdef DEVELOPMENT

	// Engineのインスタンスを取得
	auto engine = Engine::GetInstance();

	if (!ImGui::Begin("モデルエディタ"))
	{
		ImGui::End();
		return;
	}

	// モデルエディタのウィンドウ（子ウィンドウ含む）がホバーされているか判定
	bool isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

	// ドロップされたファイルの処理
	{
		std::lock_guard<std::mutex> lock(dropMutex_);
		if (!droppedFilesQueue_.empty())
		{
			if (isHovered)
			{
				std::vector<std::string> modelPathsToLoad;

				if (!std::filesystem::exists(kDir))
					std::filesystem::create_directories(kDir);

				for (const auto& originalPath : droppedFilesQueue_)
				{
					std::filesystem::path srcPath(originalPath);
					std::filesystem::path destPath = kDir + srcPath.filename().string();

					try
					{
						// フォルダがドロップされた場合の処理
						if (std::filesystem::is_directory(srcPath))
						{
							// フォルダごと再帰的にコピー
							std::filesystem::copy(srcPath, destPath, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);

							// フォルダ内からメインのモデルファイルを探す
							for (const auto& entry : std::filesystem::recursive_directory_iterator(destPath))
							{
								if (entry.is_regular_file())
								{
									std::string ext = entry.path().extension().string();
									std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

									if (ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx")
									{
										modelPathsToLoad.push_back(entry.path().string());
									}
								}
							}
						}
						else
						{
							// ファイルが単体/複数ドロップされた場合の処理
							std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing);

							std::string ext = srcPath.extension().string();
							std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

							if (ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx")
							{
								modelPathsToLoad.push_back(destPath.string());
							}
						}
					}
					catch (const std::filesystem::filesystem_error& e)
					{
						if (engine) engine->Log(LogLevel::Error, "ファイル/フォルダのコピーに失敗しました : " + std::string(e.what()));
					}
				}

				// 見つかったモデルをロード
				for (const auto& loadPath : modelPathsToLoad)
				{
					ModelHandle handle = renderContext->LoadModel(loadPath);
					if (handle.IsValid())
					{
						selectedHandle_ = handle;
					}
				}
			}
			droppedFilesQueue_.clear();
		}
	}

	ImGui::Text("ロード済み モデル");

	// 下部に削除ボタンを置くため、少し余白を残してリストのサイズを自動調整
	ImGui::BeginChild("モデル一覧", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5.0f), true);

	// モデルストアからモデルの数を取得
	size_t modelCount = modelStore->GetModelCount();
	for (uint32_t i = 0; i < modelCount; ++i)
	{
		// モデルハンドルを取得
		ModelHandle handle = modelStore->GetHandleByIndex(i);
		if (!handle.IsValid()) continue;

		// モデルデータを取得してパス等を確認する
		ModelData* modelData = modelStore->GetModelData(handle);
		if (!modelData || modelData->filePath.empty()) continue;

		std::string filePath = modelData->filePath;
		size_t slashPos = filePath.find_last_of("/\\");
		std::string fileName = (slashPos != std::string::npos) ? filePath.substr(slashPos + 1) : filePath;

		bool isSelected = (selectedHandle_ == handle);
		if (ImGui::Selectable((fileName + "##" + std::to_string(i)).c_str(), isSelected))
		{
			selectedHandle_ = handle;
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
	if (selectedHandle_.IsValid())
	{
		if (ImGui::Button("選択したモデルを削除", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			try
			{
				ModelData* data = modelStore->GetModelData(selectedHandle_);
				if (data)
				{
					std::string path = data->filePath;
					if (std::filesystem::exists(path))
						std::filesystem::remove(path);
				}

				// モデルストアから削除
				modelStore->Unload(selectedHandle_, engine->GetFrameIndex());

				// 選択状態をリセット
				selectedHandle_ = ModelHandle::Invalid();
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

	ImGui::End();

#endif
}

/// @brief ドロップされたファイルを処理する
/// @param filePath 
void Detail::ModelEditor::OnFileDropped(const std::string& filePath)
{
	std::lock_guard<std::mutex> lock(dropMutex_);
	droppedFilesQueue_.push_back(filePath);
}