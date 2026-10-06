#include "ShaderEditor.h"
#include "Engine.h"
#include "Func/ConvertString/ConvertString.h"

/// @brief コンストラクタ
Detail::ShaderEditor::ShaderEditor()
{
	// 初期化時にフォルダ内のファイルリストを更新
	RefreshFileList();
}

/// @brief 指定されたファイルを開く
/// @param filePath 
/// @return 
bool Detail::ShaderEditor::OpenFile(const std::wstring& filePath)
{
	if (!std::filesystem::exists(filePath))
		return false;

	currentFilePath_ = filePath;
	currentFilePathUtf8_ = ConvertString(filePath);

	// ファイルリストの中から現在のインデックスを同期
	selectedFileIndex_ = -1;
	for (size_t i = 0; i < fileList_.size(); ++i)
	{
		if (fileList_[i] == currentFilePath_)
		{
			selectedFileIndex_ = static_cast<int>(i);
			break;
		}
	}

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

	ImGui::Begin("シェーダエディタ");

	// ファイル選択プルダウン
	ImGui::Text("File:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-140.0f); // ボタン用のエリアを考慮して幅調整

	std::string previewName = (selectedFileIndex_ >= 0 && selectedFileIndex_ < static_cast<int>(fileListUtf8_.size()))
		? fileListUtf8_[selectedFileIndex_]
		: "選択しているファイル";

	if (ImGui::BeginCombo("##シェーダ", previewName.c_str()))
	{
		for (int n = 0; n < static_cast<int>(fileListUtf8_.size()); n++)
		{
			const bool isSelected = (selectedFileIndex_ == n);
			if (ImGui::Selectable(fileListUtf8_[n].c_str(), isSelected))
			{
				selectedFileIndex_ = n;
				OpenFile(fileList_[n]); // プルダウン選択時にそのままファイルを開く
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SameLine();

	// リスト再読み込みボタン
	if (ImGui::Button("シェーダ再読み込み"))
	{
		RefreshFileList();
	}

	ImGui::Separator();

	if (isDirty_)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[ 未保存の変更 ]");
		ImGui::SameLine();
	}

	// 保存ボタンとショートカット(Ctrl+S)
	if (ImGui::Button("保存 & コンパイル") || (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)))
	{
		if (SaveFile())
		{
			auto engine = Engine::GetInstance();
			if (engine) engine->Log(LogLevel::Info, "シェーダファイルを保存しました : " + currentFilePathUtf8_);

			// ファイル名に ".VS." が含まれていれば頂点シェーダー、そうでなければピクセルシェーダーとしてコンパイル
			std::wstring profile = (currentFilePath_.find(L".VS.") != std::wstring::npos) ? L"vs_6_0" : L"ps_6_0";
			std::string source(textBuffer_.data());

			ShaderCompileResult result = compiler->CompileSource(source, currentFilePath_, profile.c_str());

			// コンパイル結果のログを表示
			if (result.success)
			{
				compileLog_ = "コンパイル成功";
				isCompileError_ = false;
			}
			else
			{
				compileLog_ = "コンパイルエラー : \n" + result.errorMessage;
				isCompileError_ = true;
			}
		}
		else
		{
			compileLog_ = "エラー : ファイルの保存に失敗しました";
			isCompileError_ = true;
		}
	}

	// テキストエディタ領域
	ImGui::Separator();

	// エディタ領域の高さをコンパイルログ領域用に少し下を開けて計算
	float logHeight = 120.0f;
	ImVec2 editorSize(-1.0f, -logHeight);

	ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;

	// テキストバッファが空の場合、初期サイズを確保
	if (textBuffer_.empty())
	{
		textBuffer_.assign(1024 * 1024, 0);
	}

	// テキストの描画
	if (ImGui::InputTextMultiline("##シェーダコード", textBuffer_.data(), textBuffer_.size(), editorSize, flags))
	{
		isDirty_ = true;
	}

	// 下部：コンパイル出力 / エラー表示ログ
	ImGui::Separator();
	ImGui::Text("コンパイル結果 :");

	// コンパイル結果のログを色分けして表示
	if (isCompileError_)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", compileLog_.c_str());
	}
	else
	{
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", compileLog_.c_str());
	}

	ImGui::End();

#endif
}

/// @brief フォルダ内のファイルリストを更新する
void Detail::ShaderEditor::RefreshFileList()
{
	fileList_.clear();
	fileListUtf8_.clear();
	selectedFileIndex_ = -1;

	std::filesystem::path dirPath(kDir);
	if (!std::filesystem::exists(dirPath)) return;

	// 再帰的にディレクトリを探索してファイルパスを取得
	for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
	{
		if (entry.is_regular_file())
		{
			std::wstring pathW = entry.path().wstring();
			std::string pathUtf8 = ConvertString(pathW);

			fileList_.push_back(pathW);
			fileListUtf8_.push_back(pathUtf8);

			// 現在選択中のファイルと一致する場合はインデックスを保存
			if (pathW == currentFilePath_)
			{
				selectedFileIndex_ = static_cast<int>(fileList_.size() - 1);
			}
		}
	}
}