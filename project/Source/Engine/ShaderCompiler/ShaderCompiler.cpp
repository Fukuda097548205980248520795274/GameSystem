#include "ShaderCompiler.h"
#include "Func/ConvertString/ConvertString.h"
#include <cassert>
#include <format>

#include "Engine.h"

/// @brief 初期化
/// @param log 
void Detail::ShaderCompiler::Initialize()
{
	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	/*-----------------
		DXCの初期化
	-----------------*/

	// DXCの初期化
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));
	if (engine)engine->Log(LogLevel::Info, "DXCの初期化");

	// DXCコンパイラの初期化
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));
	if (engine)engine->Log(LogLevel::Info, "DXCコンパイラの初期化");

	// DXCインクルードハンドラの初期化
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
	if (engine)engine->Log(LogLevel::Info, "DXCインクルードハンドラの初期化");
}

/// @brief コンパイルする
/// @param filePath 
/// @param profile 
/// @return 
Microsoft::WRL::ComPtr<IDxcBlob> Detail::ShaderCompiler::Compile(const std::wstring& filePath, const wchar_t* profile)
{
	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	/*----------------------s
		HLSLファイルを読む
	----------------------*/

	// コンパイルをするシェーダの情報をログに出力する
	if (engine)engine->Log(LogLevel::Info, ConvertString(std::format(L"コンパイル開始 , パス : {} , プロファイル : {}", filePath, profile)));

	// HLSLファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));
	if (engine)engine->Log(LogLevel::Info, "読み込み完了");

	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;
	if (engine)
	{
		engine->Log(LogLevel::Info, std::format("シェーダソース ポインタ : {}, シェーダソース サイズ : {} bytes, シェーダソース エンコーディング : DXC_CP_UTF8", shaderSourceBuffer.Ptr, shaderSourceBuffer.Size, shaderSourceBuffer.Encoding));

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
		if (engine)engine->Log(LogLevel::Error, shaderError->GetStringPointer());
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
	if (engine)engine->Log(LogLevel::Info, ConvertString(std::format(L"コンパイル成功 , パス : {} , プロファイル : {}", filePath, profile)));

	// リソースを解放
	shaderSource->Release();
	shaderResult->Release();

	return shaderBlob;
}

/// @brief メモリ上の文字列からコンパイルする
/// @param source 
/// @param sourceName 
/// @param profile 
/// @param entryPoint 
/// @return 
Detail::ShaderCompileResult Detail::ShaderCompiler::CompileSource(const std::string& source, const std::wstring& sourceName, const wchar_t* profile, const wchar_t* entryPoint)
{
	ShaderCompileResult result;
	auto engine = Engine::GetInstance();

	if (!dxcUtils_ || !dxcCompiler_) {
		result.errorMessage = "DXC Compiler is not initialized.";
		return result;
	}

	// 1. ソースコードからDxcBufferを作成
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = source.c_str();
	shaderSourceBuffer.Size = source.size();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	// 2. コンパイル引数の構築
	std::vector<LPCWSTR> arguments = {
		sourceName.c_str(),      // ソース名
		L"-E", entryPoint,       // エントリーポイント
		L"-T", profile,          // プロファイル
		L"-Zi", L"-Qembed_debug",// デバッグ情報
		L"-Od",                  // 最適化オフ
		L"-Zpr"                  // 行優先
	};

	// 3. コンパイル実行
	Microsoft::WRL::ComPtr<IDxcResult> dxcResult;
	HRESULT hr = dxcCompiler_->Compile(
		&shaderSourceBuffer,
		arguments.data(),
		static_cast<UINT32>(arguments.size()),
		includeHandler_.Get(),
		IID_PPV_ARGS(&dxcResult)
	);

	if (FAILED(hr)) {
		result.errorMessage = "DXC Compile API Call Failed (HRESULT: " + std::to_string(hr) + ")";
		return result;
	}

	// 4. エラー・警告ログの取得
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	dxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() > 0) {
		result.errorMessage = shaderError->GetStringPointer();
	}

	// 5. コンパイルステータス（成功判定）の確認
	HRESULT status;
	dxcResult->GetStatus(&status);
	if (FAILED(status)) {
		result.success = false;
		if (engine) engine->Log(LogLevel::Error, "Shader Compile Failed: " + ConvertString(sourceName));
		return result; // エラーメッセージが result.errorMessage に入った状態で返す
	}

	// 6. バイナリ取得
	hr = dxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&result.blob), nullptr);
	if (SUCCEEDED(hr)) {
		result.success = true;
		if (engine) engine->Log(LogLevel::Info, "Shader Compiled Successfully: " + ConvertString(sourceName));
	}

	return result;
}

/// @brief ファイルからコンパイルする
/// @param filePath 
/// @param profile 
/// @param entryPoint 
/// @return 
Detail::ShaderCompileResult Detail::ShaderCompiler::CompileFile(const std::wstring& filePath, const wchar_t* profile, const wchar_t* entryPoint)
{
	ShaderCompileResult result;

	// ファイルを読み込んで文字列化
	std::ifstream file(filePath, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		result.errorMessage = "Failed to open file: " + ConvertString(filePath);
		return result;
	}

	std::ostringstream ss;
	ss << file.rdbuf();
	std::string source = ss.str();

	// CompileSource に委譲
	return CompileSource(source, filePath, profile, entryPoint);
}