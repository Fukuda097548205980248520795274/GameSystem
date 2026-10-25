#include "AudioSystem.h"

/// @brief 初期化
void Detail::AudioSystem::Initialize()
{
	// オーディオストアの生成
	audioStore_ = std::make_unique<AudioStore>();
}

/// @brief 更新処理
void Detail::AudioSystem::Update()
{
	// オーディオストアの更新
	audioStore_->Update();
}