#pragma once
#include "AudioData.h"
#include <list>
#include <wrl.h>

namespace Detail
{
	/// @brief オーディオストア
	class AudioStore
	{
	public:
		
		/// @brief コンストラクタ
		AudioStore() { Initialize(); }

		/// @brief デストラクタ
		~AudioStore();

		/// @brief 更新処理
		void Update();

		/// @brief ファイルを読む
		/// @param filePath 
		/// @return 
		AudioHandle Load(const std::string& filePath);

		/// @brief 音声を流す
		/// @param handle 
		/// @param volume 
		/// @return 
		PlayHandle PlayAudio(AudioHandle hAudio, float volume, bool isLoop = false);

		/// @brief ファイルパスを取得する
		/// @param handle 
		/// @return 
		std::string GetFilePath(AudioHandle hAudio) const { return audioTable_[hAudio.index]->filePath; }

		/// @brief 音声を停止する
		/// @param handle 
		void StopAudio(PlayHandle hPlay);

		/// @brief 音声が流れているかどうか
		/// @param handle 
		/// @return 
		bool IsAudioPlay(PlayHandle hPlay);

		/// @brief 音量を設定する
		/// @param handle 
		/// @param volume 
		void SetVolume(PlayHandle hPlay, float volume);

		/// @brief ピッチを設定する
		/// @param handle 
		/// @param pitch 
		void SetPitch(PlayHandle hPlay, float pitch);

		// Microsoft::WRL:: 省略
		template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;


	private:

		/// @brief 初期化
		void Initialize();

		/// @brief ハンドルに対応するプレイデータを探す
		/// @param handle 
		/// @return 
		PlayData* FindPlayData(PlayHandle hPlay);

		/// @brief ハンドルに対応するオーディオデータを探す
		/// @param handle 
		/// @return 
		const AudioData* FindAudioData(AudioHandle hAudio) const;

		/// @brief ユニークなプレイハンドルを生成する 
		PlayHandle GenerateUniquePlayHandle() const;

		/// @brief 音量を制御する
		/// @param volume 
		/// @return 
		static float ClampVolume(float volume);


		/// @brief パスを正規化する
		/// @param path 
		/// @return 
		std::string NormalizePath(const std::string& path) const;

		/// @brief ファイルのハッシュ値を計算する
		/// @param filePath
		/// @return
		uint64_t ComputeFileHash(const std::string& filePath) const;



		// XAudio2
		ComPtr<IXAudio2> xAudio2_ = nullptr;

		// マスターボイス
		IXAudio2MasteringVoice* masterVoice_ = nullptr;


	private:

		// オーディオテーブル
		std::vector<std::unique_ptr<AudioData>> audioTable_;

		// プレイテーブル
		std::list<std::unique_ptr<PlayData>> playTable_;

		/// @brief 出力用ディレクトリ
		const std::string kCookedDir = "./Cooked/Sound/";
	};
}