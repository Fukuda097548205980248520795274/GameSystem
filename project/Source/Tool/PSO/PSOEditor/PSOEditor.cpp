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

	// --- 右カラム：プロパティ編集 ---
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

		// ステータス表示
		ImGui::Text("Status: %s", currentItem.statusMessage.c_str());

		ImGui::Separator();

		// シェーダファイルのパス設定 & ファイル実在チェック
		bool vsExists = !currentDesc.vsPath.empty() && std::filesystem::exists(currentDesc.vsPath);
		bool psExists = !currentDesc.psPath.empty() && std::filesystem::exists(currentDesc.psPath);

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

		// ルートパラメータの設定
		if (ImGui::CollapsingHeader("Root Parameters", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button("Add Parameter")) {
				currentDesc.rootParameters.push_back(CustomRootParameter{});
				currentItem.isDirty = true;
			}

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

				int reg = static_cast<int>(p.shaderRegister);
				if (ImGui::InputInt("Register", &reg)) {
					p.shaderRegister = static_cast<uint32_t>(std::max(0, reg));
					currentItem.isDirty = true;
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
		}

		// ブレンドステート設定
		if (ImGui::CollapsingHeader("Blend State", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const char* blendItems[] = { "None", "Normal", "Add", "Subtract", "Multiply" };
			int currentBlend = static_cast<int>(currentDesc.blendMode);
			if (ImGui::Combo("Blend Mode", &currentBlend, blendItems, _countof(blendItems))) {
				currentDesc.blendMode = static_cast<BlendMode>(currentBlend);
				currentItem.isDirty = true;
			}
		}

		// --- 再構築処理 ---
		bool manualBuildPressed = ImGui::Button("Build / Rebuild PSO");
		bool shouldBuild = manualBuildPressed || (isAutoRebuild_ && currentItem.isDirty);

		if (shouldBuild)
		{
			// ファイル存在チェックによるガード
			if (!vsExists || !psExists)
			{
				currentItem.isBuildFailed = true;
				currentItem.statusMessage = "Build Failed: Missing shader file(s)";
				if (engine) engine->Log(LogLevel::Error, "Cannot build PSO. Invalid shader path: " + currentItem.name);
			}
			else
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
				}
				else
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