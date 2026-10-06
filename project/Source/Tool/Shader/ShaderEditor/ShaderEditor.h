#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace Detail
{
	class ShaderCompiler;

	class ShaderEditor
	{
	public:

		/// @brief コンストラクタ
		ShaderEditor();

		/// @brief 指定されたファイルを開く
		/// @param filePath 
		/// @return 
		bool OpenFile(const std::wstring& filePath);

		/// @brief 現在編集中のファイルを保存する
		/// @return 
		bool SaveFile();

		/// @brief UIを描画する
		/// @param compiler 
		void DrawUI(ShaderCompiler* compiler);

		/// @brief フォルダ内のファイルリストを更新する
		void RefreshFileList();

		/// @brief 現在編集中のファイルパスを取得する
		/// @return 
		const std::wstring& GetCurrentFilePath() const { return currentFilePath_; }

	private:

		/// @brief 現在編集中のファイルの内容を読み込む
		/// @return 
		bool LoadFileContent();

		// ファイルパス管理
		std::wstring currentFilePath_;
		std::string currentFilePathUtf8_;

		// フォルダ内のファイルリスト管理
		std::vector<std::wstring> fileList_;
		std::vector<std::string> fileListUtf8_;
		int selectedFileIndex_ = -1;

		// テキストバッファ (ImGuiInputTextMultiline用)
		std::vector<char> textBuffer_;


	private:

		// エラーログ関連
		std::string compileLog_;
		bool isCompileError_ = false;
		bool isDirty_ = false;

		// デフォルトのシェーダディレクトリ
		const std::string kDir = "./Assets/Shader/";
	};
}