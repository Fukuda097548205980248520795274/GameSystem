#include "ShaderCompiler.hpp"
#include "Func/ConvertString/ConvertString.hpp"
#include <cassert>
#include <format>

#include "Engine.hpp"

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
	auto engine = Engine::GetInstance();

	// Cookedディレクトリの準備
	if (!std::filesystem::exists(kCookedDir))
	{
		std::filesystem::create_directories(kCookedDir);
	}

	// CSOファイルのパスを構築
	std::filesystem::path hlslPath(filePath);
	std::string csoFileName = hlslPath.stem().string() + "_" + ConvertString(profile) + ".cso";
	std::string csoPathStr = kCookedDir + csoFileName;
	std::filesystem::path csoPath(csoPathStr);

	// CSOが存在し、かつHLSLより新しい場合はCSOを読み込む
	if (std::filesystem::exists(csoPath) && std::filesystem::exists(hlslPath))
	{
		if (std::filesystem::last_write_time(csoPath) >= std::filesystem::last_write_time(hlslPath))
		{
			if (engine) engine->Log(LogLevel::Info, "CSOキャッシュを読み込みます: " + csoPathStr);

			IDxcBlobEncoding* csoBlob = nullptr;
			std::wstring wCsoPath = ConvertString(csoPathStr);
			HRESULT hr = dxcUtils_->LoadFile(wCsoPath.c_str(), nullptr, &csoBlob);
			if (SUCCEEDED(hr))
			{
				return csoBlob;
			}
		}
	}

	if (engine) engine->Log(LogLevel::Info, ConvertString(std::format(L"コンパイル開始 , パス : {} , プロファイル : {}", filePath, profile)));

	// HLSLファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));

	// DxcBufferの構築
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	// コンパイル引数の構築
	LPCWSTR arguments[] = {
		filePath.c_str(), L"-E", L"main", L"-T", profile, L"-Zi", L"-Qembed_debug", L"-Od", L"-Zpr"
	};

	// コンパイル実行
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler_->Compile(&shaderSourceBuffer, arguments, _countof(arguments), includeHandler_.Get(), IID_PPV_ARGS(&shaderResult));
	assert(SUCCEEDED(hr));

	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		if (engine) engine->Log(LogLevel::Error, shaderError->GetStringPointer());
		assert(false);
	}

	// コンパイルステータスの確認
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	// コンパイル成功後、CSOとしてファイルに保存する
	std::ofstream csoFile(csoPathStr, std::ios::binary);
	if (csoFile.is_open())
	{
		csoFile.write(reinterpret_cast<const char*>(shaderBlob->GetBufferPointer()), shaderBlob->GetBufferSize());
		csoFile.close();
		if (engine) engine->Log(LogLevel::Info, "CSOファイルを保存しました: " + csoPathStr);
	}

	if (engine) engine->Log(LogLevel::Info, ConvertString(std::format(L"コンパイル成功 , パス : {} , プロファイル : {}", filePath, profile)));

	// リソース解放
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

	if (!dxcUtils_ || !dxcCompiler_)
	{
		result.errorMessage = "DXC Compiler is not initialized.";
		return result;
	}

	// ソースコードからDxcBufferを作成
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = source.c_str();
	shaderSourceBuffer.Size = source.size();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	// コンパイル引数の構築
	std::vector<LPCWSTR> arguments = {
		sourceName.c_str(),      // ソース名
		L"-E", entryPoint,       // エントリーポイント
		L"-T", profile,          // プロファイル
		L"-Zi", L"-Qembed_debug",// デバッグ情報
		L"-Od",                  // 最適化オフ
		L"-Zpr"                  // 行優先
	};

	// コンパイル実行
	Microsoft::WRL::ComPtr<IDxcResult> dxcResult;
	HRESULT hr = dxcCompiler_->Compile(
		&shaderSourceBuffer,
		arguments.data(),
		static_cast<UINT32>(arguments.size()),
		includeHandler_.Get(),
		IID_PPV_ARGS(&dxcResult)
	);

	if (FAILED(hr))
	{
		result.errorMessage = "DXC Compile API Call Failed (HRESULT: " + std::to_string(hr) + ")";
		return result;
	}

	// エラー・警告ログの取得
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	dxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() > 0) {
		result.errorMessage = shaderError->GetStringPointer();
	}

	// コンパイルステータス（成功判定）の確認
	HRESULT status;
	dxcResult->GetStatus(&status);
	if (FAILED(status)) {
		result.success = false;
		if (engine) engine->Log(LogLevel::Error, "Shader Compile Failed: " + ConvertString(sourceName));
		return result; // エラーメッセージが result.errorMessage に入った状態で返す
	}

	// バイナリ取得
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
	auto engine = Engine::GetInstance();

	// Cookedディレクトリの準備
	if (!std::filesystem::exists(kCookedDir))
		std::filesystem::create_directories(kCookedDir);

	// ファイルパスをstd::filesystem::pathに変換
	std::filesystem::path hlslPath(filePath);

	// プロファイルとエントリーポイントをファイル名に含めて競合を防ぐ
	std::string csoFileName = hlslPath.stem().string() + "_" + ConvertString(entryPoint) + "_" + ConvertString(profile) + ".cso";
	std::string csoPathStr = kCookedDir + csoFileName;
	std::filesystem::path csoPath(csoPathStr);

	// CSO判定
	if (std::filesystem::exists(csoPath) && std::filesystem::exists(hlslPath))
	{
		if (std::filesystem::last_write_time(csoPath) >= std::filesystem::last_write_time(hlslPath))
		{
			if (engine) engine->Log(LogLevel::Info, "CSOキャッシュを読み込みます: " + csoPathStr);

			// CSOファイルを読み込む
			IDxcBlobEncoding* csoBlob = nullptr;
			std::wstring wCsoPath = ConvertString(csoPathStr);
			HRESULT hr = dxcUtils_->LoadFile(wCsoPath.c_str(), nullptr, &csoBlob);

			// 成功した場合は結果を返す
			if (SUCCEEDED(hr))
			{
				result.success = true;
				result.blob = csoBlob;
				return result;
			}
		}
	}

	// キャッシュがない、または古い場合は通常通り文字列化してCompileSourceへ
	std::ifstream file(filePath, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		result.errorMessage = "Failed to open file: " + ConvertString(filePath);
		return result;
	}

	std::ostringstream ss;
	ss << file.rdbuf();
	std::string source = ss.str();

	// CompileSourceを呼び出す
	result = CompileSource(source, filePath, profile, entryPoint);

	// 成功したらCSOファイルを保存する
	if (result.success && result.blob)
	{
		std::ofstream csoFile(csoPathStr, std::ios::binary);
		if (csoFile.is_open())
		{
			csoFile.write(reinterpret_cast<const char*>(result.blob->GetBufferPointer()), result.blob->GetBufferSize());
			csoFile.close();
		}
	}

	return result;
}