#include "DXFence.h"
#include <cassert>

#include "Engine.h"

/// @brief デストラクタ
Detail::DXFence::~DXFence()
{
	// イベントハンドルを閉じる
	CloseHandle(fenceEvent_);
}

/// @brief 初期化
/// @param device 
void Detail::DXFence::Initialize(ID3D12Device* device)
{
	// nullptrチェック
	assert(device);

	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();


	/*------------------
		フェンスの生成
	------------------*/

	// 最大バッファ数を取得
	int32_t maxBufferCount = 1;
	if (engine) maxBufferCount = static_cast<int32_t>(engine->GetMaxBufferCount());
	fenceValues_.resize(maxBufferCount, 0);

	// フェンスの生成
	HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	if (!SUCCEEDED(hr))
	{
		// フェンス生成失敗のログ
		if (engine)engine->Log(LogLevel::Error, "フェンスの生成に失敗しました");
		throw std::runtime_error("フェンスの生成に失敗しました");
	}

	// フェンス生成成功のログ
	if (engine)engine->Log(LogLevel::Info, "フェース生成");



	/*-------------------
		イベントの生成
	-------------------*/

	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(fenceEvent_ == nullptr)
	{
		// イベント生成失敗のログ
		if (engine)engine->Log(LogLevel::Error, "イベントの生成に失敗しました");
		throw std::runtime_error("イベントの生成に失敗しました");
	}

	// イベント生成成功のログ
	if (engine)engine->Log(LogLevel::Info, "イベント生成");
}


/// @brief GPUにシグナルを送る
/// @param commandQueue 
void Detail::DXFence::SendSignal(ID3D12CommandQueue* commandQueue)
{
	assert(commandQueue);

	// フェンスの値をインクリメントする
	currentFenceValue_++;

	// フレームインデックスを取得して、フェンスの値を保持する配列に格納する
	uint32_t frameIndex = Engine::GetInstance()->GetFrameIndex();
	fenceValues_[frameIndex] = currentFenceValue_;

	// GPUにシグナルを送る
	commandQueue->Signal(fence_.Get(), currentFenceValue_);
}

/// @brief GPUの処理が完了するまで待機する
void Detail::DXFence::WaitGPU()
{
	// フレームインデックスを取得する
	uint32_t frameIndex = Engine::GetInstance()->GetFrameIndex();

	// フェンスの値を取得する
	uint64_t waitValue = fenceValues_[frameIndex];

	// フェンスの値が指定したシグナル値にたどりついているか確認する
	if (fence_->GetCompletedValue() < waitValue)
	{
		// 指定したシグナル値にたどり着いていないので、たどり着くまで待つようにイベントを設定する
		fence_->SetEventOnCompletion(waitValue, fenceEvent_);

		// イベントを待つ
		WaitForSingleObject(fenceEvent_, INFINITE);
	}
}