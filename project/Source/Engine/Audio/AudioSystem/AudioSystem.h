#pragma once
#include "../AudioStore/AudioStore.h"
#include "ECS/EntityECS/EntityECS.h"

namespace Detail
{
	class RegistryECS;

	/// @brief オーディオシステム
	class AudioSystem
	{
	public:

		/// @brief コンストラクタ
		AudioSystem() { Initialize(); }

		/// @brief デストラクタ
		~AudioSystem() = default;

		/// @brief 更新処理
		void Update();

		/// @brief オーディオエンティティを作成する
		/// @param audioHandle 
		/// @param volume 
		/// @param pitch 
		/// @param isLoop 
		/// @return 
		Entity CreateAudioEntity(Detail::AudioHandle audioHandle, float volume = 0.5f, float pitch = 1.0f, bool isLoop = false);

		/// @brief 読み込み
		/// @param filePath 
		/// @return 
		AudioHandle Load(const std::string& filePath) { return audioStore_->Load(filePath); }

		/// @brief 再生
		/// @param entity 
		/// @return 
		PlayHandle PlayAudio(Entity entity);

		/// @brief 停止
		void StopAudio(Entity entity);


	private:

		/// @brief 初期化
		void Initialize();

		/// @brief オーディオストア
		std::unique_ptr<AudioStore> audioStore_;

		/// @brief ECSレジストリ
		RegistryECS* registry_ = nullptr;
	};
}