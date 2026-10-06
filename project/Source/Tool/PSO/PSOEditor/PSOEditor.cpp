#include "PSOEditor.h"
#include "Engine.h"
#include "Func/ConvertString/ConvertString.h"

/// @brief UIを描画する
/// @param device 
/// @param compiler 
void Detail::PSOEditor::DrawUI(ID3D12Device* device, ShaderCompiler* compiler)
{
#ifdef DEVELOPMENT

	ImGui::Begin("PSO Dynamic Editor");
	auto engine = Engine::GetInstance();

	// 1. PSOの追加ボタンとAuto Rebuildトグル
	if (ImGui::Button("Create New PSO"))
	{
		PSOItem newItem;
		newItem.name = "PSO_" + std::to_string(psoItems_.size());
		newItem.isDirty = false; // 新規作成時は自動構築しない
		psoItems_.push_back(newItem);
		selectedIndex_ = static_cast<int>(psoItems_.size()) - 1;
	}

	ImGui::SameLine();
	ImGui::Checkbox("Auto Rebuild on Change", &isAutoRebuild_);

	ImGui::Separator();

	// UIを2カラムに分割 (左: リスト, 右: プロパティ)
	ImGui::Columns(2, "PSOEditorColumns", true);
	ImGui::SetColumnWidth(0, 160.0f);

	// --- 左カラム：PSOリスト ---
	for (int i = 0; i < static_cast<int>(psoItems_.size()); ++i)
	{
		bool isSelected = (selectedIndex_ == i);
		std::string label = psoItems_[i].name;

		// 状態をラベルに付与
		if (psoItems_[i].pso == nullptr) {
			label += " [Unbuilt]";
		}
		else if (psoItems_[i].isBuildFailed) {
			label += " [Error]";
		}

		if (ImGui::Selectable(label.c_str(), isSelected)) {
			selectedIndex_ = i;
		}
	}

	ImGui::NextColumn();

	if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(psoItems_.size()))
	{
		auto& currentItem = psoItems_[selectedIndex_];
		auto& currentDesc = currentItem.desc;

		// 識別名の編集
		char nameBuf[256];
		strcpy_s(nameBuf, currentItem.name.c_str());
		if (ImGui::InputText("PSO Name", nameBuf, sizeof(nameBuf))) {
			currentItem.name = nameBuf;
		}

		// PSO Type の選択を追加
		const char* psoTypeNames[] = { "Graphics", "Compute" };
		int currentTypeIdx = static_cast<int>(currentDesc.type);
		if (ImGui::Combo("PSO Type", &currentTypeIdx, psoTypeNames, _countof(psoTypeNames)))
		{
			currentDesc.type = static_cast<Detail::PSOType>(currentTypeIdx);
			currentItem.isDirty = true;
		}

		// ステータス表示
		ImGui::Text("Status: %s", currentItem.statusMessage.c_str());
		ImGui::Separator();


		// シェーダファイルのパス設定 & ファイル実在チェック
		bool vsExists = !currentDesc.vsPath.empty() && std::filesystem::exists(currentDesc.vsPath);
		bool psExists = !currentDesc.psPath.empty() && std::filesystem::exists(currentDesc.psPath);
		bool csExists = !currentDesc.csPath.empty() && std::filesystem::exists(currentDesc.csPath); // 追加

		// --- Graphics 専用のUI ---
		if (currentDesc.type == Detail::PSOType::Graphics)
		{
			if (ImGui::CollapsingHeader("Shader Files", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// 頂点シェーダ
				std::string vsPathStr = ConvertString(currentDesc.vsPath);
				char vsPathBuf[256];
				strcpy_s(vsPathBuf, vsPathStr.c_str());
				if (ImGui::InputText("VS Path", vsPathBuf, sizeof(vsPathBuf))) {
					currentDesc.vsPath = ConvertString(vsPathBuf);
					currentItem.isDirty = true;
				}
				if (!vsExists) {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  ↳ Warning: Vertex shader file not found!");
				}

				// ピクセルシェーダ
				std::string psPathStr = ConvertString(currentDesc.psPath);
				char psPathBuf[256];
				strcpy_s(psPathBuf, psPathStr.c_str());
				if (ImGui::InputText("PS Path", psPathBuf, sizeof(psPathBuf))) {
					currentDesc.psPath = ConvertString(psPathBuf);
					currentItem.isDirty = true;
				}
				if (!psExists) {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  ↳ Warning: Pixel shader file not found!");
				}
			}
		}
		// --- Compute 専用のUI ---
		else if (currentDesc.type == Detail::PSOType::Compute)
		{
			if (ImGui::CollapsingHeader("Compute Shader File", ImGuiTreeNodeFlags_DefaultOpen))
			{
				std::string csPathStr = ConvertString(currentDesc.csPath);
				char csPathBuf[256];
				strcpy_s(csPathBuf, csPathStr.c_str());
				if (ImGui::InputText("CS Path", csPathBuf, sizeof(csPathBuf))) {
					currentDesc.csPath = ConvertString(csPathBuf);
					currentItem.isDirty = true;
				}
				if (!csExists) {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  ↳ Warning: Compute shader file not found!");
				}
			}
		}

		// ルートパラメータの設定 (Graphics / Compute 共通)
		if (ImGui::CollapsingHeader("Root Parameters", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// ルートパラメータの追加ボタン
			if (ImGui::Button("Add Parameter")) 
			{
				currentDesc.rootParameters.push_back(CustomRootParameter{});
				currentItem.isDirty = true;
			}

			// ルートパラメータのリストを表示
			for (size_t i = 0; i < currentDesc.rootParameters.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));
				auto& p = currentDesc.rootParameters[i];

				const char* typeItems[] = { "CBV", "SRV", "UAV", "DescriptorTable" };
				int currentType = static_cast<int>(p.type);
				if (ImGui::Combo("Type", &currentType, typeItems, _countof(typeItems))) {
					p.type = static_cast<Detail::CustomRootParamType>(currentType);
					currentItem.isDirty = true;
				}

				// Descriptor Table 以外は単一のレジスタとして扱う
				if (p.type != Detail::CustomRootParamType::DescriptorTable)
				{
					int reg = static_cast<int>(p.shaderRegister);
					if (ImGui::InputInt("Register", &reg)) {
						p.shaderRegister = static_cast<uint32_t>(std::max(0, reg));
						currentItem.isDirty = true;
					}
				}
				else
				{
					// DescriptorTableの場合、複数のレンジを追加できるUI
					if (ImGui::Button("Add Descriptor Range"))
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

						const char* rangeTypeItems[] = { "SRV", "UAV", "CBV", "SAMPLER" };
						int currentRangeType = static_cast<int>(range.rangeType);
						if (ImGui::Combo("Range Type", &currentRangeType, rangeTypeItems, _countof(rangeTypeItems))) 
						{
							range.rangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(currentRangeType);
							currentItem.isDirty = true;
						}

						int numDesc = static_cast<int>(range.numDescriptors);
						if (ImGui::InputInt("Num Descriptors", &numDesc)) 
						{
							range.numDescriptors = static_cast<uint32_t>(std::max(1, numDesc));
							currentItem.isDirty = true;
						}

						int baseReg = static_cast<int>(range.baseShaderRegister);
						if (ImGui::InputInt("Base Register", &baseReg))
						{
							range.baseShaderRegister = static_cast<uint32_t>(std::max(0, baseReg));
							currentItem.isDirty = true;
						}

						if (ImGui::Button("Remove Range"))
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
				if (ImGui::Button("Remove Parameter")) 
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

		// 静的サンプラーの設定 (Graphics / Compute 共通)
		if (ImGui::CollapsingHeader("Static Samplers", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// サンプラーの追加ボタン
			if (ImGui::Button("Add Sampler"))
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
				if (ImGui::InputInt("Register (s#)", &reg)) {
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
				if (ImGui::Combo("Filter", &currentFilterIdx, filterItems, _countof(filterItems))) {
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
				if (ImGui::Combo("Comparison Func", &currentCmpIdx, cmpFuncItems, _countof(cmpFuncItems))) {
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

				DrawAddressCombo("Address U", sampler.addressU);
				DrawAddressCombo("Address V", sampler.addressV);
				DrawAddressCombo("Address W", sampler.addressW);

				// サンプラーの削除ボタン
				if (ImGui::Button("Remove Sampler"))
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

		// --- 以下のステートは Graphics の場合のみ表示 ---
		if (currentDesc.type == Detail::PSOType::Graphics)
		{
			// インプットレイアウトの設定
			if (ImGui::CollapsingHeader("Input Layout", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::Button("Add Input Element")) {
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
					if (ImGui::InputText("Semantic", nameBuf, sizeof(nameBuf))) {
						elem.semanticName = nameBuf;
						currentItem.isDirty = true;
					}

					ImGui::SameLine();

					// Semantic Index の編集
					int semIndex = static_cast<int>(elem.semanticIndex);
					ImGui::SetNextItemWidth(80.0f);
					if (ImGui::InputInt("Index", &semIndex)) {
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
					for (int f = 0; f < _countof(formatValues); ++f) {
						if (elem.format == formatValues[f]) { currentFormatIdx = f; break; }
					}
					ImGui::SetNextItemWidth(180.0f);
					if (ImGui::Combo("Format", &currentFormatIdx, formatNames, _countof(formatNames))) {
						elem.format = formatValues[currentFormatIdx];
						currentItem.isDirty = true;
					}

					// 削除ボタン
					ImGui::SameLine();
					if (ImGui::Button("Remove")) {
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
			if (ImGui::CollapsingHeader("Rasterizer State", ImGuiTreeNodeFlags_DefaultOpen))
			{
				const char* cullItems[] = { "None", "Front", "Back" };
				int currentCull = static_cast<int>(currentDesc.cullMode) - 1;
				if (ImGui::Combo("Cull Mode", &currentCull, cullItems, _countof(cullItems))) {
					currentDesc.cullMode = static_cast<D3D12_CULL_MODE>(currentCull + 1);
					currentItem.isDirty = true;
				}

				const char* fillItems[] = { "Wireframe", "Solid" };
				int currentFill = static_cast<int>(currentDesc.fillMode) - 2;
				if (ImGui::Combo("Fill Mode", &currentFill, fillItems, _countof(fillItems))) {
					currentDesc.fillMode = static_cast<D3D12_FILL_MODE>(currentFill + 2);
					currentItem.isDirty = true;
				}
			}

			// デプスステンシル設定
			if (ImGui::CollapsingHeader("Depth Stencil State", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::Checkbox("Depth Enable", &currentDesc.depthEnable)) currentItem.isDirty = true;

				bool depthWrite = (currentDesc.depthWriteMask == D3D12_DEPTH_WRITE_MASK_ALL);
				if (ImGui::Checkbox("Depth Write", &depthWrite)) {
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
				if (ImGui::Combo("Depth Func", &currentDepthFuncIdx, depthFuncItems, _countof(depthFuncItems)))
				{
					currentDesc.depthFunc = depthFuncValues[currentDepthFuncIdx];
					currentItem.isDirty = true;
				}
			}

			// ブレンドステート設定
			if (ImGui::CollapsingHeader("Blend State", ImGuiTreeNodeFlags_DefaultOpen))
			{
				const char* blendItems[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen", "Custom" };
				int currentBlend = static_cast<int>(currentDesc.blendMode);

				// 1. プリセット選択
				if (ImGui::Combo("Preset", &currentBlend, blendItems, _countof(blendItems))) {
					currentDesc.blendMode = static_cast<EditorBlendMode>(currentBlend);

					// Custom 以外が選ばれた場合は、プリセットのデフォルト値を customBlend に同期
					if (currentDesc.blendMode != EditorBlendMode::Custom)
					{
						currentDesc.customBlend = DynamicPSOBuilder::GetPresetBlendDesc(currentDesc.blendMode);
					}
					currentItem.isDirty = true;
				}

				ImGui::Separator();

				// 2. 詳細設定 (Custom Settings)
				auto& cb = currentDesc.customBlend;
				bool changed = false;

				if (ImGui::Checkbox("Blend Enable", &cb.blendEnable)) changed = true;

				if (cb.blendEnable)
				{
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

					auto DrawBlendCombo = [&](const char* label, D3D12_BLEND& blendVal) {
						int currentIdx = 0;
						for (int i = 0; i < _countof(blendOptionValues); ++i) {
							if (blendVal == blendOptionValues[i]) { currentIdx = i; break; }
						}
						if (ImGui::Combo(label, &currentIdx, blendOptionNames, _countof(blendOptionNames))) {
							blendVal = blendOptionValues[currentIdx];
							return true;
						}
						return false;
						};

					auto DrawOpCombo = [&](const char* label, D3D12_BLEND_OP& opVal) {
						int currentIdx = 0;
						for (int i = 0; i < _countof(opValues); ++i) {
							if (opVal == opValues[i]) { currentIdx = i; break; }
						}
						if (ImGui::Combo(label, &currentIdx, opNames, _countof(opNames))) {
							opVal = opValues[currentIdx];
							return true;
						}
						return false;
						};

					// RGB 設定
					if (ImGui::TreeNode("Color (RGB) Blend"))
					{
						changed |= DrawBlendCombo("Src Blend RGB", cb.srcBlend);
						changed |= DrawBlendCombo("Dest Blend RGB", cb.destBlend);
						changed |= DrawOpCombo("Blend Op RGB", cb.blendOp);
						ImGui::TreePop();
					}

					// Alpha 設定
					if (ImGui::TreeNode("Alpha Blend"))
					{
						changed |= DrawBlendCombo("Src Blend Alpha", cb.srcBlendAlpha);
						changed |= DrawBlendCombo("Dest Blend Alpha", cb.destBlendAlpha);
						changed |= DrawOpCombo("Blend Op Alpha", cb.blendOpAlpha);
						ImGui::TreePop();
					}
				}

				// 詳細項目を変更した場合は自動的に Mode を Custom に切り替えて Rebuild フラグを立てる
				if (changed)
				{
					currentDesc.blendMode = EditorBlendMode::Custom;
					currentItem.isDirty = true;
				}
			}
		}

		// --- 再構築処理 ---
		bool manualBuildPressed = ImGui::Button("Build / Rebuild PSO");
		bool shouldBuild = manualBuildPressed || (isAutoRebuild_ && currentItem.isDirty);

		if (shouldBuild)
		{
			// タイプに応じたファイル存在チェック
			bool canBuild = false;
			if (currentDesc.type == Detail::PSOType::Graphics) {
				canBuild = vsExists && psExists;
			} else {
				canBuild = csExists;
			}

			if (!canBuild)
			{
				currentItem.isBuildFailed = true;
				currentItem.statusMessage = "Build Failed: Missing shader file(s)";
				if (engine) engine->Log(LogLevel::Error, "Cannot build PSO. Invalid shader path: " + currentItem.name);
			} else
			{
				DynamicPSOBuilder builder;
				Microsoft::WRL::ComPtr<ID3D12PipelineState> newPSO;
				Microsoft::WRL::ComPtr<ID3D12RootSignature> newRootSig;

				if (builder.Build(device, compiler, currentDesc, &newRootSig, &newPSO))
				{
					currentItem.pso = newPSO;
					currentItem.rootSig = newRootSig;
					currentItem.isBuildFailed = false;
					currentItem.statusMessage = "Successfully Built";
					if (engine) engine->Log(LogLevel::Info, "PSO Rebuilt Successfully: " + currentItem.name);
				} else
				{
					currentItem.isBuildFailed = true;
					currentItem.statusMessage = "Compile Error";
					if (engine) engine->Log(LogLevel::Error, "Failed to Rebuild PSO: " + currentItem.name);
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