#include "ShaderCompiler.h"
#include "Func/ConvertString/ConvertString.h"
#include <cassert>
#include <format>

/// @brief 初期化
/// @param log 
void Detail::ShaderCompiler::Initialize()
{
	/*-----------------
		DXCの初期化
	-----------------*/

	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));
	if (log_)log_->Logging("DxcCreateInstance IDxcUtils \n");

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));
	if (log_)log_->Logging("DxcCreateInstance IDxcCompiler3 \n");

	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
	if (log_)log_->Logging("CreateDefaultIncludeHandler \n");
}

/// @brief コンパイルする
/// @param filePath 
/// @param profile 
/// @return 
Microsoft::WRL::ComPtr<IDxcBlob> Detail::ShaderCompiler::Compile(const std::wstring& filePath, const wchar_t* profile)
{
	/*----------------------
		HLSLファイルを読む
	----------------------*/

	// コンパイルをするシェーダの情報をログに出力する
	if (log_)log_->Logging(ConvertString(std::format(L"Begin CompileShader , path : {} , profile : {}", filePath, profile)));

	// HLSLファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));
	if (log_)log_->Logging("Succeeded Load");

	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;
	if (log_)
	{
		log_->Logging(std::format("ShaderSource Ptr : {}", shaderSourceBuffer.Ptr));
		log_->Logging(std::format("ShaderSource Size : {} bytes", shaderSourceBuffer.Size));
		log_->Logging("ShaderSource Encoding : DXC_CP_UTF8");
	}


	/*------------------
		コンパイルする
	------------------*/

	LPCWSTR arguments[] =
	{
		// コンパイル対象のHLSLファイル名
		filePath.c_str(),

		// エントリーポイントの指定
		L"-E", L"main",

		// ShaderProfileの設定
		L"-T", profile,

		// デバッグ用の情報を埋め込む
		L"-Zi", L"-Qembed_debug",

		// 最適化を外す
		L"-Od",

		// メモリレイアウトは行優先
		L"-Zpr"
	};

	// 実際にシェーダをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler_->Compile(
		// 読み込んだファイル
		&shaderSourceBuffer,

		// コンパイルオプション
		arguments,

		//　コンパイルオプションの数
		_countof(arguments),

		// includeが含まれた諸々
		includeHandler_.Get(),

		// コンパイル結果
		IID_PPV_ARGS(&shaderResult)
	);

	assert(SUCCEEDED(hr));


	/*----------------------------------
		警告・エラーがでていないか確認する
	----------------------------------*/

	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		// エラーをログに出力する
		if (log_)log_->Logging(shaderError->GetStringPointer());
		assert(false);
	}


	/*-------------------------------
		コンパイル結果を受け取って返す
	-------------------------------*/

	// コンパイル結果から実行用のバイナリ部分を取得する
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	// コンパイル成功ログ
	if (log_)log_->Logging("Succeeded Compiler \n");

	// リソースを解放
	shaderSource->Release();
	shaderResult->Release();

	return shaderBlob;
}

