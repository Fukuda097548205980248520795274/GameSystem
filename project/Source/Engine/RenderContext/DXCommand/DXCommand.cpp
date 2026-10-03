#include "DXCommand.h"
#include <cassert>

#include "Engine.h"

/// @brief 初期化
/// @param device 
void Detail::DXCommand::Initialize(ID3D12Device* device)
{
	// nullptrチェック
	assert(device);

	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();

	/*---------------------------
		コマンドキューを生成する
	---------------------------*/

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};

	HRESULT hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	if (!SUCCEEDED(hr))
	{
		// コマンドキュー生成失敗のログ
		if (engine)engine->Log(LogLevel::Error, "コマンドキューの生成に失敗しました");
		throw std::runtime_error("コマンドキューの生成に失敗しました");
	}

	// コマンドキュー生成成功のログ
	if (engine)engine->Log(LogLevel::Info, "コマンドキュー生成");



	/*-----------------------------
		コマンドアロケータを生成する
	-----------------------------*/

	int32_t maxBufferCount = 1;
	if (engine)maxBufferCount = static_cast<int32_t>(engine->GetMaxBufferCount());
	commandAllocators_.resize(maxBufferCount);

	for (int i = 0; i < maxBufferCount; ++i)
	{
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators_[i]));
		if (!SUCCEEDED(hr))
		{
			// コマンドアロケータ生成失敗のログ
			if (engine)engine->Log(LogLevel::Error, "コマンドアロケータの生成に失敗しました");
			throw std::runtime_error("コマンドアロケータの生成に失敗しました");
		}

		// コマンドアロケータ生成成功のログ
		if (engine)engine->Log(LogLevel::Info, "コマンドアロケータ生成 : " + std::to_string(i));
	}

	/*--------------------------
		コマンドリストを生成する
	--------------------------*/

	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators_[0].Get(), nullptr, IID_PPV_ARGS(&commandList_));
	if (!SUCCEEDED(hr))
	{
		// コマンドリスト生成失敗のログ
		if (engine)engine->Log(LogLevel::Error, "コマンドリストの生成に失敗しました");
		throw std::runtime_error("コマンドリストの生成に失敗しました");
	}

	// コマンドリストは生成直後は記録状態なので、Closeしておく
	commandList_->Close();

	// コマンドリスト生成成功のログ
	if (engine)engine->Log(LogLevel::Info, "コマンドリスト生成");
}

/// @brief コマンドアロケータを取得する
/// @return 
ID3D12CommandAllocator* Detail::DXCommand::GetCommandAllocator()const
{
	int32_t frameIndex = static_cast<int32_t>(Engine::GetInstance()->GetFrameIndex());
	return commandAllocators_[frameIndex].Get();
}