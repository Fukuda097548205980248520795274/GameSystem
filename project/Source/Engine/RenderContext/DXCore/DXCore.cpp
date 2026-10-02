#include "DXCore.h"
#include <format>

#include "Engine.h"
#include "Func/ConvertString/ConvertString.h"

/// @brief コンストラクタ
Detail::DXCore::DXCore()
{
	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();


	/*--------------------------
		DXGIファクトリーの生成
	--------------------------*/

	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
	if (!SUCCEEDED(hr))
	{
		// 失敗ログ出力
		if (engine)engine->Log(LogLevel::Error, "DXGIファクトリーの生成に失敗しました");
		throw std::runtime_error("DXGIファクトリーの生成に失敗しました");
	}

	// 成功ログ出力
	if (engine)engine->Log(LogLevel::Info, "DXGIファクトリー生成");


	/*-----------------------------------
		使用するアダプタ（GPU）を決定する
	-----------------------------------*/

	// 良い順にアダプタを頼む
	for (UINT i = 0;
		dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		// アダプタ情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter_->GetDesc3(&adapterDesc);
		if (!SUCCEEDED(hr))
		{
			if (engine)engine->Log(LogLevel::Error, "アダプタ情報の取得に失敗しました");
			throw std::runtime_error("アダプタ情報の取得に失敗しました");
		}

		// ソフトウェアアダプタなら使わない
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			// 採用したアダプタ情報をログに出力
			if (engine)engine->Log(LogLevel::Info, ConvertString(std::format(L"使用アダプタ : {}", adapterDesc.Description)));
			break;
		}

		useAdapter_ = nullptr;
	}

	// アダプタが見つからなかったら止める
	if (useAdapter_ == nullptr)
	{
		if (engine)engine->Log(LogLevel::Error, "使用するアダプタが見つかりませんでした");
		throw std::runtime_error("使用するアダプタが見つかりませんでした");
	}


	/*----------------------
		デバイスを生成する
	----------------------*/

	// 機能レベル
	D3D_FEATURE_LEVEL featureLevels[] =
	{ D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0 };

	// ログ出力用文字列
	const char* featureLevelStrings[] = { "12.2" , "12.1" , "12.0" };

	// 高い順に生成できるか試す
	for (size_t i = 0; i < _countof(featureLevels); ++i)
	{
		// 採用したアダプタでデバイスを生成
		hr = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));

		// 指定した機能レベルで生成できたか確認
		if (SUCCEEDED(hr))
		{
			// 生成したデバイスの機能レベルをログ出力
			if (engine)engine->Log(LogLevel::Info, std::format("機能レベル : {}", featureLevelStrings[i]));
			break;
		}
	}

	// デバイス生成に失敗したら止める
	if (device_ == nullptr)
	{
		if (engine)engine->Log(LogLevel::Error, "デバイスの生成に失敗しました");
		throw std::runtime_error("デバイスの生成に失敗しました");
	}

	if (engine)engine->Log(LogLevel::Info, "デバイス生成");
}