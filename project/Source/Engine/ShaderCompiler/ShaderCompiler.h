#pragma once
#include <wrl.h>
#include <string>
#include <dxcapi.h>

namespace Detail
{
	// @brief シェーダコンパイル結果
	struct ShaderCompileResult
	{
		// コンパイルが成功したかどうか
		bool success = false;

		// コンパイル結果のバイナリデータ
		Microsoft::WRL::ComPtr<IDxcBlob> blob = nullptr;

		// エラーメッセージ（コンパイル失敗時に使用）
		std::string errorMessage;
	};

	class ShaderCompiler
	{
	public:

		/// @brief 初期化
		/// @param log 
		void Initialize();

		/// @brief コンパイルする
		/// @param filePath 
		/// @param profile 
		/// @return 
		Microsoft::WRL::ComPtr<IDxcBlob> Compile(const std::wstring& filePath, const wchar_t* profile);

		/// @brief メモリ上の文字列からコンパイルする
		/// @param source 
		/// @param sourceName 
		/// @param profile 
		/// @param entryPoint 
		/// @return 
		ShaderCompileResult CompileSource(const std::string& source, const std::wstring& sourceName, const wchar_t* profile, const wchar_t* entryPoint = L"main");

		/// @brief ファイルからコンパイルする
		/// @param filePath 
		/// @param profile 
		/// @param entryPoint 
		/// @return 
		ShaderCompileResult CompileFile(const std::wstring& filePath, const wchar_t* profile, const wchar_t* entryPoint = L"main");



	private:


		// DXCユーティリティ
		Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_ = nullptr;

		// DXCコンパイラ
		Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;

		// DXCインクルードハンドラ
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;
	};
}