#pragma once
#include <wrl.h>
#include <string>
#include <dxcapi.h>

namespace Detail
{
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



	private:


		// DXCユーティリティ
		Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_ = nullptr;

		// DXCコンパイラ
		Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;

		// DXCインクルードハンドラ
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;
	};
}