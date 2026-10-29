#include "AudioSystem.hpp"
#include "Engine.hpp"

/// @brief 初期化
void Detail::AudioSystem::Initialize()
{
	auto engine = Engine::GetInstance();
	assert(engine);

	// オーディオストアの生成
	audioStore_ = std::make_unique<AudioStore>();
}

/// @brief 更新処理
void Detail::AudioSystem::Update()
{
	// オーディオストアの更新
	audioStore_->Update();
}

/// @brief オーディオエンティティを作成する
/// @param audioHandle 
/// @param volume 
/// @param pitch 
/// @param isLoop 
/// @return 
Entity Detail::AudioSystem::CreateAudioEntity(AudioHandle audioHandle, float volume, float pitch, bool isLoop)
{
	// ECSレジストリを取得する
	auto registry = Engine::GetInstance()->GetRegistryECS();
	if (!registry) return kNullEntity;

	// ECSレジストリから新しいエンティティを作成
	Entity entity = registry->CreateEntity();

	// AudioComponentを追加
	AudioComponent audioComponent;
	audioComponent.audioHandle = audioHandle;
	audioComponent.volume = volume;
	audioComponent.pitch = pitch;
	audioComponent.isLoop = isLoop;
	registry->AddComponent<AudioComponent>(entity, audioComponent);

	return entity;
}

/// @brief 再生
/// @param entity 
/// @return 
Detail::PlayHandle Detail::AudioSystem::PlayAudio(Entity entity)
{
	// ECSレジストリを取得する
	auto registry = Engine::GetInstance()->GetRegistryECS();
	if (!registry) return PlayHandle();

	auto audioComponent = registry->GetComponent<AudioComponent>(entity);
	if (!audioComponent) return PlayHandle();

	// 重複再生を許可しない場合は、現在鳴っている音をすべて停止する
	if (!audioComponent->allowOverlap)
	{
		for (auto handle : audioComponent->playHandles)
		{
			audioStore_->StopAudio(handle);
		}
		audioComponent->playHandles.clear();
	}
	else
	{
		// 重複再生する場合、すでに再生が終了した古いハンドルをリストから除外しておく（メモリ肥大化防止）
		audioComponent->playHandles.erase(
			std::remove_if(audioComponent->playHandles.begin(), audioComponent->playHandles.end(),
				[this](Detail::PlayHandle h) { return !audioStore_->IsAudioPlay(h); }),
			audioComponent->playHandles.end()
		);
	}

	// オーディオを再生
	Detail::PlayHandle newHandle = audioStore_->PlayAudio(audioComponent->audioHandle, audioComponent->volume, audioComponent->isLoop);

	// 新たな再生ハンドルが有効であれば、ピッチを設定してリストに追加
	if (newHandle.IsValid())
	{
		audioStore_->SetVolume(newHandle, audioComponent->volume);
		audioStore_->SetPitch(newHandle, audioComponent->pitch);

		// リストに追加
		audioComponent->playHandles.push_back(newHandle);
	}

	return newHandle;
}

/// @brief 停止
void Detail::AudioSystem::StopAudio(Entity entity)
{
	// ECSレジストリを取得する
	auto registry = Engine::GetInstance()->GetRegistryECS();
	if (!registry) return;

	auto audioComponent = registry->GetComponent<AudioComponent>(entity);
	if (!audioComponent) return;

	// 保持しているすべての音声を停止する
	for (Detail::PlayHandle handle : audioComponent->playHandles)
	{
		audioStore_->StopAudio(handle);
	}

	// プレイハンドルをリセット
	audioComponent->playHandles.clear();
}