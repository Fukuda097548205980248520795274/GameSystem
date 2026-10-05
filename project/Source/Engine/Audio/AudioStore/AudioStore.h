#pragma once
#include <vector>
#include <string>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <xaudio2.h>
#include <mferror.h>
#include <list>
#include <wrl.h>
#include <memory>

namespace Detail
{
	/// @brief 音声を破棄する関数オブジェクト
	struct VoiceDelete
	{
		/// @brief デリート関数
		/// @param voice 
		void operator()(IXAudio2SourceVoice* voice) const
		{
			if (voice) {
				voice->Stop(0);
				voice->DestroyVoice();
			}
		}
	};

	class AudioStore
	{
	public:

		// オーディオデータ
		struct AudioData
		{
			// @brief デストラクタ
			~AudioData();

			// ファイルパス
			std::string filePath;

			// フォーマット
			WAVEFORMATEX* waveFormat = nullptr;

			// メディアデータ
			std::vector<BYTE> mediaData;

			// サウンドハンドル
			uint32_t handle;
		};

		// プレイデータ
		struct PlayData
		{
			// ハンドル
			uint32_t handle;

			// ソースボイス
			std::unique_ptr<IXAudio2SourceVoice, VoiceDelete> pSourceVoice;
		};


	public:

		/// @brief デストラクタ
		~AudioStore();

		/// @brief 初期化
		/// @param log 
		void Initialize();

		/// @brief 更新処理
		void Update();

		/// @brief ファイルを読む
		/// @param filePath 
		/// @return 
		uint32_t Load(const std::string& filePath);

		/// @brief 音声を流す
		/// @param handle 
		/// @param volume 
		/// @return 
		uint32_t PlayAudio(uint32_t hAudio, float volume, bool isLoop = false);

		/// @brief ファイルパスを取得する
		/// @param handle 
		/// @return 
		std::string GetFilePath(uint32_t hAudio) const { return audioTable_[hAudio]->filePath; }

		/// @brief 音声を停止する
		/// @param handle 
		void StopAudio(uint32_t hPlay);

		/// @brief 音声が流れているかどうか
		/// @param handle 
		/// @return 
		bool IsAudioPlay(uint32_t hPlay);

		/// @brief 音量を設定する
		/// @param handle 
		/// @param volume 
		void SetVolume(uint32_t hPlay, float volume);

		/// @brief ピッチを設定する
		/// @param handle 
		/// @param pitch 
		void SetPitch(uint32_t hPlay, float pitch);

		// Microsoft::WRL:: 省略
		template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;


	private:

		/// @brief ハンドルに対応するプレイデータを探す
		/// @param handle 
		/// @return 
		PlayData* FindPlayData(uint32_t hPlay);

		/// @brief ハンドルに対応するオーディオデータを探す
		/// @param handle 
		/// @return 
		const AudioData* FindAudioData(uint32_t hAudio) const;

		/// @brief ユニークなプレイハンドルを生成する 
		uint32_t GenerateUniquePlayHandle() const;

		/// @brief 音量を制御する
		/// @param volume 
		/// @return 
		static float ClampVolume(float volume);



		// XAudio2
		ComPtr<IXAudio2> xAudio2_ = nullptr;

		// マスターボイス
		IXAudio2MasteringVoice* masterVoice_ = nullptr;


	private:

		// オーディオテーブル
		std::vector<std::unique_ptr<AudioData>> audioTable_;

		// プレイテーブル
		std::list<std::unique_ptr<PlayData>> playTable_;
	};
}