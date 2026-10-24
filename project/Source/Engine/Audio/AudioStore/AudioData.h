#pragma once
#include <vector>
#include <string>
#include <memory>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <xaudio2.h>
#include <mferror.h>

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

	// オーディオデータ
	struct AudioData
	{
		// @brief デストラクタ
		~AudioData()
		{
			// フォーマットを解放する
			if (waveFormat)
			{
				CoTaskMemFree(waveFormat);
				waveFormat = nullptr;
			}

			// メディアデータをクリアする
			mediaData.clear();
		}

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
}