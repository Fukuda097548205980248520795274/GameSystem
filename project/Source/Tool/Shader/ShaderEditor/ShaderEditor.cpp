#include "ShaderEditor.h"
#include "Engine.h"
#include "Func/ConvertString/ConvertString.h"

/// @brief 指定されたファイルを開く
/// @param filePath 
/// @return 
bool Detail::ShaderEditor::OpenFile(const std::wstring& filePath)
{
	if (!std::filesystem::exists(filePath)) return false;

	currentFilePath_ = filePath;
	currentFilePathUtf8_ = ConvertString(filePath);
	return LoadFileContent();
}

/// @brief 現在編集中のファイルの内容を読み込む
/// @return 
bool Detail::ShaderEditor::LoadFileContent()
{
	std::ifstream file(currentFilePath_, std::ios::in | std::ios::binary);
	if (!file.is_open()) return false;

	std::ostringstream ss;
	ss << file.rdbuf();
	std::string content = ss.str();

	// バッファサイズを余分（1MB程度）に確保しておく
	size_t capacity = std::max(content.size() * 2, static_cast<size_t>(1024 * 1024));
	textBuffer_.assign(capacity, 0);
	std::copy(content.begin(), content.end(), textBuffer_.begin());

	isDirty_ = false;
	compileLog_ = "Ready";
	isCompileError_ = false;
	return true;
}

/// @brief 現在編集中のファイルを保存する
/// @return 
bool Detail::ShaderEditor::SaveFile()
{
	if (currentFilePath_.empty()) return false;

	std::ofstream file(currentFilePath_, std::ios::out | std::ios::binary);
	if (!file.is_open()) return false;

	// バッファ内の文字列を書き込み
	file.write(textBuffer_.data(), strlen(textBuffer_.data()));
	file.close();

	isDirty_ = false;
	return true;
}

/// @brief UIを描画する
/// @param compiler 
void Detail::ShaderEditor::DrawUI(ShaderCompiler* compiler)
{
#ifdef DEVELOPMENT

	ImGui::Begin("Shader Source Editor");

	// 1. ファイル選択・読み込みヘッダー
	char pathBuf[512];
	strcpy_s(pathBuf, currentFilePathUtf8_.c_str());

	ImGui::Text("File:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-120.0f);
	if (ImGui::InputText("##ShaderPath", pathBuf, sizeof(pathBuf))) {
		currentFilePathUtf8_ = pathBuf;
		currentFilePath_ = ConvertString(currentFilePathUtf8_);
	}
	ImGui::SameLine();

	if (ImGui::Button("Open")) {
		if (!OpenFile(currentFilePath_)) {
			compileLog_ = "Error: Failed to open file.";
			isCompileError_ = true;
		}
	}

	ImGui::Separator();

	// 2. コントロールボタン（Save & Compile）
	if (isDirty_) {
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[ Unsaved Changes ]");
		ImGui::SameLine();
	}

	if (ImGui::Button("Save & Test Compile") || (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)))
	{
		if (SaveFile())
		{
			auto engine = Engine::GetInstance();
			if (engine) engine->Log(LogLevel::Info, "Shader Saved: " + currentFilePathUtf8_);

			// ファイル名に ".VS." が含まれていれば頂点シェーダー、そうでなければピクセルシェーダーとしてコンパイル
			std::wstring profile = (currentFilePath_.find(L".VS.") != std::wstring::npos) ? L"vs_6_0" : L"ps_6_0";
			std::string source(textBuffer_.data());

			ShaderCompileResult result = compiler->CompileSource(source, currentFilePath_, profile.c_str());

			if (result.success)
			{
				compileLog_ = "File Saved and Compiled Successfully.";
				isCompileError_ = false;
			} 
			else
			{
				compileLog_ = "Compile Error:\n" + result.errorMessage;
				isCompileError_ = true;
			}
		} 
		else
		{
			compileLog_ = "Error: Failed to save file.";
			isCompileError_ = true;
		}
	}

	// 3. テキストエディタ領域
	ImGui::Separator();

	// エディタ領域の高さをコンパイルログ領域用に少し下を開けて計算
	float logHeight = 120.0f;
	ImVec2 editorSize(-1.0f, -logHeight);

	ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;

	if (textBuffer_.empty()) {
		textBuffer_.assign(1024 * 1024, 0); // 初期バッファ確保
	}

	if (ImGui::InputTextMultiline("##ShaderCode", textBuffer_.data(), textBuffer_.size(), editorSize, flags))
	{
		isDirty_ = true;
	}

	// 4. 下部：コンパイル出力 / エラー表示ログ
	ImGui::Separator();
	ImGui::Text("Compile Output:");

	if (isCompileError_) {
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", compileLog_.c_str());
	} else {
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", compileLog_.c_str());
	}

	ImGui::End();

#endif
}