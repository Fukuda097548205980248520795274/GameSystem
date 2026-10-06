#include "PSOEditor.h"
#include "Engine.h"
#include "Func/ConvertString/ConvertString.h"

/// @brief コンストラクタ
Detail::PSOEditor::PSOEditor()
{
	RefreshPsoFileList();
	RefreshShaderFileList();
}

/// @brief 更新処理
void Detail::PSOEditor::Update()
{
#ifdef DEVELOPMENT

	auto engine = Engine::GetInstance();

	// 安全に破棄できるフレーム数を取得
	uint32_t safeFrameAge = 2;
	if (engine) safeFrameAge = engine->GetMaxBufferCount();

	// 古いPSOやルートシグネチャを破棄するためのキューを更新
	for (auto it = garbageQueue_.begin(); it != garbageQueue_.end(); )
	{
		it->frameAge++;
		if (it->frameAge >= safeFrameAge)
		{
			// イテレータ削除時の ComPtr デストラクタ呼び出しにより、ここで安全に Release される
			it = garbageQueue_.erase(it);
		}
		else
		{
			++it;
		}
	}

#endif
}

/// @brief UIを描画する
/// @param device 
/// @param compiler 
void Detail::PSOEditor::DrawUI(ID3D12Device* device, ShaderCompiler* compiler)
{
#ifdef DEVELOPMENT

	ImGui::Begin("PSO動的エディタ");
	auto engine = Engine::GetInstance();

	// PSOデータファイルのプルダウン描画
	std::string psoFilePreview = (selectedPsoFileIndex_ >= 0 && selectedPsoFileIndex_ < static_cast<int>(psoFileList_.size()))
		? psoFileList_[selectedPsoFileIndex_]
		: "ファイルを選択してください...";

	ImGui::SetNextItemWidth(250.0f);
	bool isComboOpen = ImGui::BeginCombo("PSOデータファイル", psoFilePreview.c_str());

	if (isComboOpen)
	{
		// 開いた最初の1フレーム（瞬間）のみファイルリストをリフレッシュ
		if (!wasComboOpen_)
		{
			RefreshPsoFileList();

			// 現在選択・編集中のファイル名とリスト内のインデックスを再同期
			selectedPsoFileIndex_ = -1;
			for (int i = 0; i < static_cast<int>(psoFileList_.size()); ++i)
			{
				if (std::filesystem::path(psoFileList_[i]).stem().string() == saveFileNameBuffer_)
				{
					selectedPsoFileIndex_ = i;
					break;
				}
			}
		}

		for (int i = 0; i < static_cast<int>(psoFileList_.size()); ++i)
		{
			bool isSelected = (selectedPsoFileIndex_ == i);

			if (ImGui::Selectable(psoFileList_[i].c_str(), isSelected))
			{
				selectedPsoFileIndex_ = i;

				// 選択されたファイル名を保存用バッファにコピー
				saveFileNameBuffer_ = std::filesystem::path(psoFileList_[i]).stem().string();

				// プルダウンで選択されたら自動でファイルを読み込む
				LoadFromFile(saveFileNameBuffer_.c_str());
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	// 現在の開閉状態を保持（次のフレームで判定に使用）
	wasComboOpen_ = isComboOpen;

	ImGui::SameLine();



	// 保存ファイル名の入力欄を追加
	char fileBuf[256];
	strcpy_s(fileBuf, saveFileNameBuffer_.c_str());
	ImGui::SetNextItemWidth(150.0f);
	if (ImGui::InputText("保存ファイル名", fileBuf, sizeof(fileBuf)))
	{
		saveFileNameBuffer_ = fileBuf;
	}
	ImGui::SameLine();

	// 保存ボタンの有効化条件をチェック
	bool canSave = (saveFileNameBuffer_.size() > 0);
	if (!canSave) ImGui::BeginDisabled();
	if (ImGui::Button("保存"))
	{
		// 入力されたファイル名で保存し、リストを更新する
		SaveToFile(saveFileNameBuffer_.c_str());
		RefreshPsoFileList();

		// 保存したファイルをリストから検索し、選択状態（プルダウン）を更新
		for (int i = 0; i < static_cast<int>(psoFileList_.size()); ++i)
		{
			if (std::filesystem::path(psoFileList_[i]).stem().string() == saveFileNameBuffer_)
			{
				selectedPsoFileIndex_ = i;
				break;
			}
		}
	}
	if (!canSave) ImGui::EndDisabled();

	ImGui::SameLine();


	// PSOの追加ボタンとAuto Rebuildトグル
	if (ImGui::Button("新規作成 PSO"))
	{
		PSOItem newItem;
		newItem.name = "PSO_" + std::to_string(psoItems_.size());

		// 保存ファイル名が空（ファイル未選択状態）の場合のみ、デフォルトの保存名としてセット
		if (saveFileNameBuffer_.empty())
		{
			saveFileNameBuffer_ = newItem.name;
		}

		newItem.isDirty = false; // 新規作成時は自動構築しない
		psoItems_.push_back(newItem);
		selectedIndex_ = static_cast<int>(psoItems_.size()) - 1;
	}

	ImGui::SameLine();
	ImGui::Checkbox("変更時に自動再構築", &isAutoRebuild_);

	ImGui::Separator();

	// UIを2カラムに分割 (左: リスト, 右: プロパティ)
	ImGui::Columns(2, "PSOEditorColumns", true);
	ImGui::SetColumnWidth(0, 160.0f);

	// 左カラム：PSOリスト
	for (int i = 0; i < static_cast<int>(psoItems_.size()); ++i)
	{
		bool isSelected = (selectedIndex_ == i);
		std::string label = psoItems_[i].name;

		// 状態をラベルに付与
		if (psoItems_[i].pso == nullptr)
		{
			label += " [未 ビルド]";
		}
		else if (psoItems_[i].isBuildFailed)
		{
			label += " [エラー]";
		}

		// 選択可能なリストアイテムとして表示
		if (ImGui::Selectable(label.c_str(), isSelected))
		{
			selectedIndex_ = i;
		}
	}

	ImGui::NextColumn();

	// 右カラム：選択されたPSOのプロパティ
	if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(psoItems_.size()))
	{
		auto& currentItem = psoItems_[selectedIndex_];
		auto& currentDesc = currentItem.desc;

		// 識別名の編集
		char nameBuf[256];
		strcpy_s(nameBuf, currentItem.name.c_str());
		if (ImGui::InputText("PSO 名", nameBuf, sizeof(nameBuf)))
		{
			currentItem.name = nameBuf;
		}

		// PSO Type の選択を追加
		const char* psoTypeNames[] = { "グラフィックス", "コンピュート" };
		int currentTypeIdx = static_cast<int>(currentDesc.type);
		if (ImGui::Combo("PSO タイプ", &currentTypeIdx, psoTypeNames, _countof(psoTypeNames)))
		{
			currentDesc.type = static_cast<Detail::PSOType>(currentTypeIdx);
			currentItem.isDirty = true;
		}

		// ステータス表示
		ImGui::Text("ステータス: %s", currentItem.statusMessage.c_str());
		ImGui::Separator();


		// シェーダファイルのパス設定 & ファイル実在チェック
		bool vsExists = !currentDesc.vsPath.empty() && std::filesystem::exists(currentDesc.vsPath);
		bool psExists = !currentDesc.psPath.empty() && std::filesystem::exists(currentDesc.psPath);
		bool csExists = !currentDesc.csPath.empty() && std::filesystem::exists(currentDesc.csPath);

		// シェーダファイルのコンボボックス描画用ラムダ関数
		auto DrawShaderCombo = [&](const char* label, std::wstring& currentPathW, bool& wasOpenFlag)
			{
				std::string currentPathUtf8 = ConvertString(currentPathW);
				std::string previewName = currentPathUtf8.empty() ? "選択してください..." : currentPathUtf8;
				bool changed = false;

				bool isComboOpen = ImGui::BeginCombo(label, previewName.c_str());
				if (isComboOpen)
				{
					// 開いた最初の1フレーム（瞬間）のみファイルリストをリフレッシュ
					if (!wasOpenFlag)
					{
						RefreshShaderFileList();
					}

					for (size_t n = 0; n < shaderFileListUtf8_.size(); n++)
					{
						bool isSelected = (currentPathW == shaderFileList_[n]);

						if (ImGui::Selectable(shaderFileListUtf8_[n].c_str(), isSelected))
						{
							currentPathW = shaderFileList_[n];
							changed = true;
						}

						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				// 現在の開閉状態を保持（次のフレームで判定に使用）
				wasOpenFlag = isComboOpen;

				return changed;
			};


		// Graphics 専用のUI
		if (currentDesc.type == Detail::PSOType::Graphics)
		{
			if (ImGui::CollapsingHeader("グラフィックス シェーダファイル", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// 頂点シェーダ
				if (DrawShaderCombo("頂点シェーダ パス", currentDesc.vsPath, wasVsComboOpen_))
				{
					currentItem.isDirty = true;
				}

				// ファイルが存在しない場合の警告表示
				if (!vsExists)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  ↳ 警告 : 頂点シェーダーファイルが見つかりません！");
				}

				// ピクセルシェーダ
				if (DrawShaderCombo("ピクセルシェーダ パス", currentDesc.psPath, wasPsComboOpen_))
				{
					currentItem.isDirty = true;
				}

				// ファイルが存在しない場合の警告表示
				if (!psExists)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), " ↳ 警告 : ピクセルシェーダーファイルが見つかりません！");
				}
			}
		}
		else if (currentDesc.type == Detail::PSOType::Compute)
		{
			// Compute 専用のUI
			if (ImGui::CollapsingHeader("コンピュート シェーダファイル", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (DrawShaderCombo("コンピュートシェーダ パス", currentDesc.csPath, wasCsComboOpen_))
				{
					currentItem.isDirty = true;
				}

				// ファイルが存在しない場合の警告表示
				if (!csExists)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), " ↳ 警告 : コンピュートシェーダーファイルが見つかりません！");
				}
			}
		}

		// ルートパラメータの設定 (Graphics / Compute 共通)
		if (ImGui::CollapsingHeader("ルートパラメータ", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// ルートパラメータの追加ボタン
			if (ImGui::Button("パラメータ 追加")) 
			{
				currentDesc.rootParameters.push_back(CustomRootParameter{});
				currentItem.isDirty = true;
			}

			// ルートパラメータのリストを表示
			for (size_t i = 0; i < currentDesc.rootParameters.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));
				auto& p = currentDesc.rootParameters[i];

				// パラメータの種類を選択するコンボボックス
				const char* typeItems[] = { "CBV", "SRV", "UAV", "DescriptorTable" };
				int currentType = static_cast<int>(p.type);
				if (ImGui::Combo("タイプ", &currentType, typeItems, _countof(typeItems))) {
					p.type = static_cast<Detail::CustomRootParamType>(currentType);
					currentItem.isDirty = true;
				}

				// Descriptor Table 以外は単一のレジスタとして扱う
				if (p.type != Detail::CustomRootParamType::DescriptorTable)
				{
					int reg = static_cast<int>(p.shaderRegister);
					if (ImGui::InputInt("レジスタ", &reg)) {
						p.shaderRegister = static_cast<uint32_t>(std::max(0, reg));
						currentItem.isDirty = true;
					}
				}
				else
				{
					// DescriptorTableの場合、複数のレンジを追加できるUI
					if (ImGui::Button("ディスクリプタレンジ 追加"))
					{
						p.descriptorRanges.push_back(CustomDescriptorRange{});
						currentItem.isDirty = true;
					}

					// Descriptor Range のリストを表示
					for (size_t j = 0; j < p.descriptorRanges.size(); ++j)
					{
						ImGui::PushID(static_cast<int>(j) + 10000); // 階層のID衝突を回避
						auto& range = p.descriptorRanges[j];

						ImGui::Indent();

						// レンジの種類を選択するコンボボックス
						const char* rangeTypeItems[] = { "SRV", "UAV", "CBV", "SAMPLER" };
						int currentRangeType = static_cast<int>(range.rangeType);
						if (ImGui::Combo("レンジタイプ", &currentRangeType, rangeTypeItems, _countof(rangeTypeItems))) 
						{
							range.rangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(currentRangeType);
							currentItem.isDirty = true;
						}

						// ディスクリプタ数の編集
						int numDesc = static_cast<int>(range.numDescriptors);
						if (ImGui::InputInt("ディスクリプタ数", &numDesc)) 
						{
							range.numDescriptors = static_cast<uint32_t>(std::max(1, numDesc));
							currentItem.isDirty = true;
						}

						// レジスタ番号の編集
						int baseReg = static_cast<int>(range.baseShaderRegister);
						if (ImGui::InputInt("レジスタ番号", &baseReg))
						{
							range.baseShaderRegister = static_cast<uint32_t>(std::max(0, baseReg));
							currentItem.isDirty = true;
						}

						// レジスタスペースの編集
						if (ImGui::Button("レンジ 削除"))
						{
							p.descriptorRanges.erase(p.descriptorRanges.begin() + j);
							currentItem.isDirty = true;
							ImGui::Unindent();
							ImGui::PopID();
							break;
						}

						ImGui::Unindent();
						ImGui::PopID();
					}
				}

				// パラメータの削除ボタン
				if (ImGui::Button("パラメータ 削除")) 
				{
					currentDesc.rootParameters.erase(currentDesc.rootParameters.begin() + i);
					currentItem.isDirty = true;
					ImGui::PopID();
					break;
				}

				ImGui::Separator();
				ImGui::PopID();
			}
		}

		// サンプラーの設定 (Graphics / Compute 共通)
		if (ImGui::CollapsingHeader("サンプラー", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// サンプラーの追加ボタン
			if (ImGui::Button("サンプラー 追加"))
			{
				currentDesc.staticSamplers.push_back(CustomStaticSampler{});
				currentItem.isDirty = true;
			}

			// サンプラーのリストを表示
			for (size_t i = 0; i < currentDesc.staticSamplers.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i) + 20000); // 階層のID衝突を回避
				auto& sampler = currentDesc.staticSamplers[i];

				// レジスタ番号の編集
				int reg = static_cast<int>(sampler.shaderRegister);
				if (ImGui::InputInt("レジスタ番号 (s#)", &reg)) {
					sampler.shaderRegister = static_cast<uint32_t>(std::max(0, reg));
					currentItem.isDirty = true;
				}

				// フィルターの編集
				const char* filterItems[] = { "MIN_MAG_MIP_POINT", "MIN_MAG_MIP_LINEAR", "ANISOTROPIC",
					"COMPARISON_MIN_MAG_MIP_POINT", "COMPARISON_MIN_MAG_MIP_LINEAR", "COMPARISON_ANISOTROPIC" };
				D3D12_FILTER filterValues[] = { D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_FILTER_ANISOTROPIC, 
					D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D12_FILTER_COMPARISON_ANISOTROPIC };
				int currentFilterIdx = 0;
				for (int f = 0; f < _countof(filterValues); ++f) {
					if (sampler.filter == filterValues[f]) { currentFilterIdx = f; break; }
				}
				if (ImGui::Combo("フィルター", &currentFilterIdx, filterItems, _countof(filterItems))) {
					sampler.filter = filterValues[currentFilterIdx];
					currentItem.isDirty = true;
				}

				// 比較関数の編集
				const char* cmpFuncItems[] = {
					"NEVER", "LESS", "EQUAL", "LESS_EQUAL",
					"GREATER", "NOT_EQUAL", "GREATER_EQUAL", "ALWAYS"
				};
				D3D12_COMPARISON_FUNC cmpFuncValues[] = {
					D3D12_COMPARISON_FUNC_NEVER, D3D12_COMPARISON_FUNC_LESS, D3D12_COMPARISON_FUNC_EQUAL, D3D12_COMPARISON_FUNC_LESS_EQUAL,
					D3D12_COMPARISON_FUNC_GREATER, D3D12_COMPARISON_FUNC_NOT_EQUAL, D3D12_COMPARISON_FUNC_GREATER_EQUAL, D3D12_COMPARISON_FUNC_ALWAYS
				};
				int currentCmpIdx = 0;
				for (int c = 0; c < _countof(cmpFuncValues); ++c) {
					if (sampler.comparisonFunc == cmpFuncValues[c]) { currentCmpIdx = c; break; }
				}
				if (ImGui::Combo("比較関数", &currentCmpIdx, cmpFuncItems, _countof(cmpFuncItems))) {
					sampler.comparisonFunc = cmpFuncValues[currentCmpIdx];
					currentItem.isDirty = true;
				}

				// アドレスモードの編集
				const char* addressModeItems[] = { "WRAP", "MIRROR", "CLAMP", "BORDER", "MIRROR_ONCE" };
				D3D12_TEXTURE_ADDRESS_MODE addressModeValues[] = {
					D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_BORDER,
					D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE
				};

				auto DrawAddressCombo = [&](const char* label, D3D12_TEXTURE_ADDRESS_MODE& mode) {
					int currentIdx = 0;
					for (int m = 0; m < _countof(addressModeValues); ++m) {
						if (mode == addressModeValues[m]) { currentIdx = m; break; }
					}
					if (ImGui::Combo(label, &currentIdx, addressModeItems, _countof(addressModeItems))) {
						mode = addressModeValues[currentIdx];
						currentItem.isDirty = true;
					}
					};

				DrawAddressCombo("アドレス U", sampler.addressU);
				DrawAddressCombo("アドレス V", sampler.addressV);
				DrawAddressCombo("アドレス W", sampler.addressW);

				// サンプラーの削除ボタン
				if (ImGui::Button("サンプラー 削除"))
				{
					currentDesc.staticSamplers.erase(currentDesc.staticSamplers.begin() + i);
					currentItem.isDirty = true;
					ImGui::PopID();
					break;
				}

				ImGui::Separator();
				ImGui::PopID();
			}
		}

		// Graphics 専用のUI
		if (currentDesc.type == Detail::PSOType::Graphics)
		{
			// インプットレイアウトの設定
			if (ImGui::CollapsingHeader("入力レイアウト", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// インプットレイアウトの追加ボタン
				if (ImGui::Button("入力レイアウト 追加"))
				{
					currentDesc.inputLayouts.push_back(CustomInputElement{});
					currentItem.isDirty = true;
				}

				for (size_t i = 0; i < currentDesc.inputLayouts.size(); ++i)
				{
					ImGui::PushID(static_cast<int>(i) + 1000); // ID衝突防止のためオフセット
					auto& elem = currentDesc.inputLayouts[i];

					// Semantic Name の編集
					char nameBuf[64];
					strcpy_s(nameBuf, elem.semanticName.c_str());
					ImGui::SetNextItemWidth(120.0f);
					if (ImGui::InputText("セマンティック", nameBuf, sizeof(nameBuf))) {
						elem.semanticName = nameBuf;
						currentItem.isDirty = true;
					}

					ImGui::SameLine();

					// Semantic Index の編集
					int semIndex = static_cast<int>(elem.semanticIndex);
					ImGui::SetNextItemWidth(80.0f);
					if (ImGui::InputInt("インデックス", &semIndex)) 
					{
						elem.semanticIndex = static_cast<uint32_t>(std::max(0, semIndex));
						currentItem.isDirty = true;
					}

					// Format の編集 (代表的なものを列挙)
					const char* formatNames[] = {
						"R32G32B32A32_FLOAT", "R32G32B32_FLOAT", "R32G32_FLOAT", "R32_FLOAT",
						"R8G8B8A8_UNORM", "R8G8B8A8_UINT"
					};
					DXGI_FORMAT formatValues[] = {
						DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32_FLOAT,
						DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UINT
					};
					int currentFormatIdx = 0;
					for (int f = 0; f < _countof(formatValues); ++f)
					{
						if (elem.format == formatValues[f]) { currentFormatIdx = f; break; }
					}
					ImGui::SetNextItemWidth(180.0f);
					if (ImGui::Combo("フォーマット", &currentFormatIdx, formatNames, _countof(formatNames)))
					{
						elem.format = formatValues[currentFormatIdx];
						currentItem.isDirty = true;
					}

					// 削除ボタン
					ImGui::SameLine();
					if (ImGui::Button("入力レイアウト 削除"))
					{
						currentDesc.inputLayouts.erase(currentDesc.inputLayouts.begin() + i);
						currentItem.isDirty = true;
						ImGui::PopID();
						break; // 要素を削除したらループを抜ける
					}

					ImGui::Separator();
					ImGui::PopID();
				}
			}

			// ラスタライザステート設定
			if (ImGui::CollapsingHeader("ラスタライザ設定", ImGuiTreeNodeFlags_DefaultOpen))
			{
				const char* cullItems[] = { "なし", "前面", "背面" };
				int currentCull = static_cast<int>(currentDesc.cullMode) - 1;
				if (ImGui::Combo("カリングモード", &currentCull, cullItems, _countof(cullItems)))
				{
					currentDesc.cullMode = static_cast<D3D12_CULL_MODE>(currentCull + 1);
					currentItem.isDirty = true;
				}

				const char* fillItems[] = { "ワイヤーフレーム", "ソリッド" };
				int currentFill = static_cast<int>(currentDesc.fillMode) - 2;
				if (ImGui::Combo("フィルモード", &currentFill, fillItems, _countof(fillItems)))
				{
					currentDesc.fillMode = static_cast<D3D12_FILL_MODE>(currentFill + 2);
					currentItem.isDirty = true;
				}
			}

			// デプスステンシル設定
			if (ImGui::CollapsingHeader("デプスステンシル設定", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::Checkbox("深度 有効", &currentDesc.depthEnable)) currentItem.isDirty = true;

				bool depthWrite = (currentDesc.depthWriteMask == D3D12_DEPTH_WRITE_MASK_ALL);
				if (ImGui::Checkbox("深度 書き込み", &depthWrite)) {
					currentDesc.depthWriteMask = depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
					currentItem.isDirty = true;
				}

				// Depth Func の編集
				const char* depthFuncItems[] = { "LESS", "LESS_EQUAL", "EQUAL", "GREATER", "GREATER_EQUAL", "ALWAYS" };
				D3D12_COMPARISON_FUNC depthFuncValues[] = {
					D3D12_COMPARISON_FUNC_LESS, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_COMPARISON_FUNC_EQUAL,
					D3D12_COMPARISON_FUNC_GREATER, D3D12_COMPARISON_FUNC_GREATER_EQUAL, D3D12_COMPARISON_FUNC_ALWAYS
				};
				int currentDepthFuncIdx = 0;
				for (int d = 0; d < _countof(depthFuncValues); ++d) 
				{
					if (currentDesc.depthFunc == depthFuncValues[d]) { currentDepthFuncIdx = d; break; }
				}
				if (ImGui::Combo("深度テスト関数", &currentDepthFuncIdx, depthFuncItems, _countof(depthFuncItems)))
				{
					currentDesc.depthFunc = depthFuncValues[currentDepthFuncIdx];
					currentItem.isDirty = true;
				}
			}

			// ブレンドステート設定
			if (ImGui::CollapsingHeader("ブレンド設定", ImGuiTreeNodeFlags_DefaultOpen))
			{
				const char* blendItems[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen", "Custom" };
				int currentBlend = static_cast<int>(currentDesc.blendMode);

				// プリセット選択
				if (ImGui::Combo("プリセット", &currentBlend, blendItems, _countof(blendItems))) 
				{
					currentDesc.blendMode = static_cast<EditorBlendMode>(currentBlend);

					// Custom 以外が選ばれた場合は、プリセットのデフォルト値を customBlend に同期
					if (currentDesc.blendMode != EditorBlendMode::Custom)
					{
						currentDesc.customBlend = DynamicPSOBuilder::GetPresetBlendDesc(currentDesc.blendMode);
					}

					currentItem.isDirty = true;
				}


				if (currentDesc.blendMode == EditorBlendMode::Custom)
				{
					// 詳細設定
					auto& cb = currentDesc.customBlend;

					// ブレンド有効のチェックボックス
					ImGui::Checkbox("ブレンド 有効", &cb.blendEnable);

					// D3D12_BLEND の定義リスト
					const char* blendOptionNames[] = {
						"ZERO", "ONE", "SRC_COLOR", "INV_SRC_COLOR", "SRC_ALPHA",
						"INV_SRC_ALPHA", "DEST_ALPHA", "INV_DEST_ALPHA", "DEST_COLOR", "INV_DEST_COLOR"
					};
					D3D12_BLEND blendOptionValues[] = {
						D3D12_BLEND_ZERO, D3D12_BLEND_ONE, D3D12_BLEND_SRC_COLOR, D3D12_BLEND_INV_SRC_COLOR, D3D12_BLEND_SRC_ALPHA,
						D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_DEST_ALPHA, D3D12_BLEND_INV_DEST_ALPHA, D3D12_BLEND_DEST_COLOR, D3D12_BLEND_INV_DEST_COLOR
					};

					// D3D12_BLEND_OP の定義リスト
					const char* opNames[] = { "ADD", "SUBTRACT", "REV_SUBTRACT", "MIN", "MAX" };
					D3D12_BLEND_OP opValues[] = {
						D3D12_BLEND_OP_ADD, D3D12_BLEND_OP_SUBTRACT, D3D12_BLEND_OP_REV_SUBTRACT, D3D12_BLEND_OP_MIN, D3D12_BLEND_OP_MAX
					};

					// D3D12_BLEND のコンボボックス描画
					auto DrawBlendCombo = [&](const char* label, D3D12_BLEND& blendVal) 
						{
							int currentIdx = 0;
							for (int i = 0; i < _countof(blendOptionValues); ++i) 
							{
								if (blendVal == blendOptionValues[i]) { currentIdx = i; break; }
							}
							if (ImGui::Combo(label, &currentIdx, blendOptionNames, _countof(blendOptionNames)))
							{
								blendVal = blendOptionValues[currentIdx];
								return true;
							}
							return false;
						};

					// D3D12_BLEND_OP のコンボボックス描画
					auto DrawOpCombo = [&](const char* label, D3D12_BLEND_OP& opVal)
						{
							int currentIdx = 0;
							for (int i = 0; i < _countof(opValues); ++i)
							{
								if (opVal == opValues[i]) { currentIdx = i; break; }
							}
							if (ImGui::Combo(label, &currentIdx, opNames, _countof(opNames))) 
							{
								opVal = opValues[currentIdx];
								return true;
							}
							return false;
						};

					// RGB 設定
					if (ImGui::TreeNode("カラー (RGB) ブレンド"))
					{
						DrawBlendCombo("ソースブレンド RGB", cb.srcBlend);
						DrawBlendCombo("デストブレンド RGB", cb.destBlend);
						DrawOpCombo("ブレンド演算 RGB", cb.blendOp);
						ImGui::TreePop();
					}

					// Alpha 設定
					if (ImGui::TreeNode("Alpha ブレンド"))
					{
						DrawBlendCombo("ソースブレンド Alpha", cb.srcBlendAlpha);
						DrawBlendCombo("デストブレンド Alpha", cb.destBlendAlpha);
						DrawOpCombo("ブレンド演算 Alpha", cb.blendOpAlpha);
						ImGui::TreePop();
					}
				}
			}
		}

		// 再構築処理
		bool manualBuildPressed = ImGui::Button("PSOビルド / 再構築");
		bool shouldBuild = manualBuildPressed || (isAutoRebuild_ && currentItem.isDirty);

		if (shouldBuild)
		{
			// タイプに応じたファイル存在チェック
			bool canBuild = false;

			// Graphics PSO の場合は VS と PS が必要、Compute PSO の場合は CS が必要
			if (currentDesc.type == Detail::PSOType::Graphics)
			{
				canBuild = vsExists && psExists;
			} 
			else 
			{
				canBuild = csExists;
			}

			// ビルド可能かどうかのチェック
			if (!canBuild)
			{
				currentItem.isBuildFailed = true;
				currentItem.statusMessage = "ビルド失敗 : シェーダファイルが見つかりません";
				if (engine) engine->Log(LogLevel::Error, "PSOビルド失敗. 無効なシェーダパス : " + currentItem.name);
			} 
			else
			{
				DynamicPSOBuilder builder;
				Microsoft::WRL::ComPtr<ID3D12PipelineState> newPSO;
				Microsoft::WRL::ComPtr<ID3D12RootSignature> newRootSig;

				// PSOのビルドを試みる
				if (builder.Build(device, compiler, currentDesc, &newRootSig, &newPSO))
				{
					// 古いPSOやルートシグネチャを破棄するためにガベージキューに追加
					if (currentItem.pso || currentItem.rootSig)
					{
						garbageQueue_.push_back({ currentItem.pso, currentItem.rootSig, 0 });
					}

					// ビルドに成功した場合の処理
					currentItem.pso = newPSO;
					currentItem.rootSig = newRootSig;
					currentItem.isBuildFailed = false;
					currentItem.statusMessage = "ビルド成功";
					if (engine) engine->Log(LogLevel::Info, "PSO ビルド成功 : " + currentItem.name);
				}
				else
				{
					// ビルドに失敗した場合の処理
					currentItem.isBuildFailed = true;
					currentItem.statusMessage = "コンパイルエラー";
					if (engine) engine->Log(LogLevel::Error, "PSO ビルド失敗 : " + currentItem.name);
				}
			}

			currentItem.isDirty = false;
		}
	}

	ImGui::Columns(1);
	ImGui::End();

#endif
}

/// @brief 名前からアクティブなPSOを取得する
/// @param name 
/// @return 
ID3D12PipelineState* Detail::PSOEditor::GetPSO(const std::string& name) const
{
	for (const auto& item : psoItems_) 
	{
		if (item.name == name) return item.pso.Get();
	}

	return nullptr;
}

/// @brief 名前からアクティブなルートシグネチャを取得する
/// @param name 
/// @return 
ID3D12RootSignature* Detail::PSOEditor::GetRootSignature(const std::string& name) const
{
	for (const auto& item : psoItems_)
	{
		if (item.name == name) return item.rootSig.Get();
	}

	return nullptr;
}

/// @brief PSO設定をJSONファイルに保存する
/// @param filepath 
void Detail::PSOEditor::SaveToFile(const std::string& filename)
{
	const std::string filePath = kDir + filename + ".json";

	json jArray = json::array();
	for (const auto& item : psoItems_)
	{
		json jItem;
		jItem["name"] = item.name;

		// 既存の ToJson 関数を呼び出して desc を変換
		json jDesc;
		ToJson(jDesc, item.desc);
		jItem["desc"] = jDesc;

		jArray.push_back(jItem);
	}

	std::ofstream file(filePath);
	if (file.is_open())
	{
		// インデント幅4で整形して出力
		file << jArray.dump(4);

		// ファイルを閉じる
		file.close();
	}
}

/// @brief JSONファイルからPSO設定を読み込む
/// @param filename 
void Detail::PSOEditor::LoadFromFile(const std::string& filename)
{
	const std::string filePath = kDir + filename + ".json";

	std::ifstream file(filePath);
	if (file.is_open())
	{
		json jArray;
		file >> jArray;

		psoItems_.clear();
		for (const auto& jItem : jArray)
		{
			PSOItem newItem;
			newItem.name = jItem.value("name", "Loaded PSO");

			if (jItem.contains("desc"))
			{
				// 既存の FromJson 関数を呼び出して desc を復元
				FromJson(jItem["desc"], newItem.desc);
			}

			// 読み込み直後は再ビルドが必要なのでフラグを立てる
			newItem.isDirty = true;
			newItem.isBuildFailed = false;
			newItem.statusMessage = "Not Built";

			psoItems_.push_back(newItem);
		}

		// psoItems_が空でない場合は最初のアイテムを選択、空の場合は選択なしにする
		if (!psoItems_.empty())
		{
			selectedIndex_ = 0;
		}
		else
		{
			selectedIndex_ = -1;
		}

		// ファイルを閉じる
		file.close();
	}
}

/// @brief 選択されているPSOファイルの名前を取得する
/// @return 
std::string Detail::PSOEditor::GetSelectedPsoFileName() const
{
	if (selectedPsoFileIndex_ >= 0 && selectedPsoFileIndex_ < static_cast<int>(psoFileList_.size()))
	{
		return std::filesystem::path(psoFileList_[selectedPsoFileIndex_]).filename().string();
	}

	return "";
}

/// @brief 選択されているPSOファイルのパスを取得する
/// @return 
std::string Detail::PSOEditor::GetSelectedPsoFilePath() const
{
	if (selectedPsoFileIndex_ >= 0 && selectedPsoFileIndex_ < static_cast<int>(psoFileList_.size()))
	{
		return psoFileList_[selectedPsoFileIndex_];
	}
	return "";
}

/// @brief PSOファイルリストを更新する
void Detail::PSOEditor::RefreshPsoFileList()
{
	psoFileList_.clear();

	std::filesystem::path dirPath(kDir);
	if (!std::filesystem::exists(dirPath))
	{
		std::filesystem::create_directories(dirPath);
		return;
	}

	// PSO保存用ディレクトリ配下の.jsonファイルを再帰走査
	for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".json")
		{
			psoFileList_.push_back(entry.path().string());
		}
	}
}

/// @brief シェーダファイルリストを更新する
void Detail::PSOEditor::RefreshShaderFileList()
{
	shaderFileList_.clear();
	shaderFileListUtf8_.clear();

	std::filesystem::path dirPath(kShaderDir);
	if (!std::filesystem::exists(dirPath)) return;

	// サブフォルダ含めてシェーダーファイルを再帰的に検索
	for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
	{
		if (entry.is_regular_file())
		{
			std::wstring pathW = entry.path().wstring();
			std::string pathUtf8 = ConvertString(pathW);

			shaderFileList_.push_back(pathW);
			shaderFileListUtf8_.push_back(pathUtf8);
		}
	}
}