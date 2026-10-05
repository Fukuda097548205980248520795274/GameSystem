#include "AudioStore.h"
#include <cassert>
#include <algorithm>
#include <limits>
#include "Func/ConvertString/ConvertString.h"
#include "Func/RandomFunc/RandomFunc.h"
#include <format>

namespace
{
	// 音声を停止して破棄する関数
	void StopAndDestroyVoice(IXAudio2SourceVoice*& sourceVoice)
	{
		if (sourceVoice)
		{
			sourceVoice->Stop(0);
			sourceVoice->DestroyVoice();
			sourceVoice = nullptr;
		}
	}

	// ピッチを制御する関数
	float ClampPitch(float pitch)
	{
		return std::clamp(pitch, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_MAX_FREQ_RATIO);
	}
}

/// @brief デストラクタ
Detail::AudioStore::AudioData::~AudioData()
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



/// @brief デストラクタ
Detail::AudioStore::~AudioStore()
{
	// 再生中の音声を停止して破棄する
	playTable_.clear();

	// MFの終了処理
	HRESULT hr = MFShutdown();
	assert(SUCCEEDED(hr));

	// XAudio2インスタンスを破棄する
	xAudio2_.Reset();
}

/// @brief 初期化
/// @param log 
void Detail::AudioStore::Initialize()
{
	// MFの初期化（ローカル版）
	HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		throw std::runtime_error("Error : MFStartup");
	}

	// XAudio2を初期化する
	hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		throw std::runtime_error("Error : XAudio2Create");
	}

	// マスターボイスを生成する
	hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		throw std::runtime_error("Error : CreateMasteringVoice");
	}
}

/// @brief 更新処理
void Detail::AudioStore::Update()
{
	// 再生中の音声を確認し、再生が終了しているものは破棄する
	playTable_.remove_if([](std::unique_ptr<PlayData>& playDatum)
		{
			// ソースボイスが存在する場合、再生状態を確認する
			if (playDatum->pSourceVoice)
			{
				// 再生状態を取得する
				XAUDIO2_VOICE_STATE state;
				playDatum->pSourceVoice->GetState(&state);

				// 再生が終了している場合は破棄するs
				if (state.BuffersQueued <= 0)
				{
					return true;
				}
			}
			else
			{
				// ソースボイスが存在しない場合は、すでに再生が終了しているとみなして破棄する
				return true;
			}

			return false;
		}
	);
}

/// @brief ファイルを読む
/// @param filePath 
/// @return 
uint32_t Detail::AudioStore::Load(const std::string& filePath)
{
	// 同じファイルパスを見つけたら、そのハンドルを返す
	for (std::unique_ptr<AudioData>& data : audioTable_)
	{
		if (filePath == data->filePath)
			return data->handle;
	}

	// wStringに変換する
	const std::wstring kFilePathW = ConvertString(filePath);

	// ソースレーダを作成する
	ComPtr<IMFSourceReader> pMFSourceReader{ nullptr };
	HRESULT hr = MFCreateSourceReaderFromURL(kFilePathW.c_str(), NULL, &pMFSourceReader);
	assert(SUCCEEDED(hr));


	// メディアタイプの作成
	ComPtr<IMFMediaType> pReader{ nullptr };
	hr = MFCreateMediaType(&pReader);
	assert(SUCCEEDED(hr));

	// ソースレーダとメディアタイプの設定
	pReader->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pReader->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	hr = pMFSourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pReader.Get());
	assert(SUCCEEDED(hr));

	// メディアタイプを解放し、再度作成する
	ComPtr<IMFMediaType> pOutType;
	hr = pMFSourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutType);
	assert(SUCCEEDED(hr));


	// オーディオデータを作成する
	std::unique_ptr<AudioData> audioDatum = std::make_unique<AudioData>();
	audioDatum->filePath = filePath;

	// サウンドハンドルを取得する
	uint32_t hAudio = static_cast<uint32_t>(audioTable_.size());
	audioDatum->handle = hAudio;

	// ウェーブフォーマットを作成する
	MFCreateWaveFormatExFromMFMediaType(pOutType.Get(), &audioDatum->waveFormat, nullptr);

	while (true)
	{
		ComPtr<IMFSample> pMFSample{ nullptr };

		DWORD streamIndex = 0;
		DWORD flags = 0;
		LONGLONG llTimeStamp = 0;

		hr = pMFSourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &pMFSample);
		assert(SUCCEEDED(hr));

		if (flags & MF_SOURCE_READERF_ENDOFSTREAM)break;

		if (pMFSample)
		{
			ComPtr<IMFMediaBuffer> pBuffer{ nullptr };
			hr = pMFSample->ConvertToContiguousBuffer(&pBuffer);
			assert(SUCCEEDED(hr));

			BYTE* pData{ nullptr };
			DWORD currentLength = 0;
			hr = pBuffer->Lock(&pData, nullptr, &currentLength);
			assert(SUCCEEDED(hr));

			audioDatum->mediaData.resize(audioDatum->mediaData.size() + currentLength);
			memcpy(audioDatum->mediaData.data() + audioDatum->mediaData.size() - currentLength, pData, currentLength);

			pBuffer->Unlock();
		}
	}

	// 配列に登録する
	audioTable_.push_back(std::move(audioDatum));

	return hAudio;
}


/// @brief オーディオを流す
/// @param handle 
/// @param volume 
/// @param isLoop 
/// @return 
uint32_t Detail::AudioStore::PlayAudio(uint32_t hAudio, float volume, bool isLoop)
{
	if (!xAudio2_) return 0;

	const AudioData* audioData = FindAudioData(hAudio);
	if (!audioData || !audioData->waveFormat || audioData->mediaData.empty())
		return 0;

	std::unique_ptr<PlayData> playDatum = std::make_unique<PlayData>();
	uint32_t hPlay = GenerateUniquePlayHandle();
	playDatum->handle = hPlay;

	IXAudio2SourceVoice* rawVoice = nullptr;
	HRESULT hr = xAudio2_->CreateSourceVoice(&rawVoice, audioData->waveFormat);

	// assert(SUCCEEDED(hr)) だけでなく、実際の処理でも弾く（リリース対策）
	if (FAILED(hr) || !rawVoice) return 0;

	// スマートポインタに管理を委譲（これ以降は自動破棄される）
	playDatum->pSourceVoice.reset(rawVoice);

	XAUDIO2_BUFFER buffer{ 0 };
	buffer.pAudioData = audioData->mediaData.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = sizeof(BYTE) * static_cast<UINT32>(audioData->mediaData.size());

	// ループ設定の追加
	if (isLoop) {
		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	hr = playDatum->pSourceVoice->SubmitSourceBuffer(&buffer);
	if (FAILED(hr)) return 0; // pSourceVoiceは自動で破棄されるため安全

	volume = ClampVolume(volume);
	hr = playDatum->pSourceVoice->SetVolume(volume);
	if (FAILED(hr)) return 0;

	hr = playDatum->pSourceVoice->Start(0);
	if (FAILED(hr)) return 0;

	playTable_.push_back(std::move(playDatum));
	return hPlay;
}


/// @brief オーディオを停止する
/// @param handle 
void Detail::AudioStore::StopAudio(uint32_t hPlay)
{
	for (auto it = playTable_.begin(); it != playTable_.end(); ++it)
	{
		if (hPlay == (*it)->handle)
		{
			// ソースボイスを停止して破棄する
			playTable_.erase(it);
			return;
		}
	}
}

/// @brief オーディオを再生されているかどうか
/// @param handle 
/// @return 
bool Detail::AudioStore::IsAudioPlay(uint32_t hPlay)
{
	PlayData* playData = FindPlayData(hPlay);
	if (!playData || !playData->pSourceVoice)
	{
		return false;
	}

	XAUDIO2_VOICE_STATE state{};
	playData->pSourceVoice->GetState(&state);

	return state.BuffersQueued > 0;
}


/// @brief ボリュームを設定する
/// @param handle 
/// @param volume 
void Detail::AudioStore::SetVolume(uint32_t hPlay, float volume)
{
	// 規格外の音にならぬようにする
	volume = ClampVolume(volume);

	// ハンドルが一致する構造体を探す
	PlayData* playData = FindPlayData(hPlay);
	if (!playData || !playData->pSourceVoice)
		return;

	HRESULT hr = playData->pSourceVoice->SetVolume(volume);
	assert(SUCCEEDED(hr));
}

/// @brief ピッチを設定する
/// @param handle 
/// @param pitch 
void Detail::AudioStore::SetPitch(uint32_t hPlay, float pitch)
{
	pitch = ClampPitch(pitch);

	// ハンドルが一致する構造体を探す
	PlayData* playData = FindPlayData(hPlay);
	if (!playData || !playData->pSourceVoice)
		return;

	HRESULT hr = playData->pSourceVoice->SetFrequencyRatio(pitch);
	assert(SUCCEEDED(hr));
}

/// @brief ハンドルに対応するプレイデータを探す
/// @param handle 
/// @return 
Detail::AudioStore::PlayData* Detail::AudioStore::FindPlayData(uint32_t hPlay)
{
	for (std::unique_ptr<PlayData>& playDatum : playTable_)
	{
		if (hPlay == playDatum->handle)
		{
			return playDatum.get();
		}
	}

	return nullptr;
}

/// @brief ハンドルに対応するオーディオデータを探す
/// @param handle 
/// @return 
const Detail::AudioStore::AudioData* Detail::AudioStore::FindAudioData(uint32_t hAudio) const
{
	if (hAudio >= audioTable_.size())
		return nullptr;

	return audioTable_[hAudio].get();
}

/// @brief ユニークなプレイハンドルを生成する
/// @return 
uint32_t Detail::AudioStore::GenerateUniquePlayHandle() const
{
	constexpr uint32_t kMaxRetryCount = 64;

	// ランダムでユニークなハンドルを生成する
	for (uint32_t retry = 0; retry < kMaxRetryCount; ++retry)
	{
		const uint32_t kHPlay = GetRandomRange(1, 10000000);
		const bool kIsDuplicate = std::any_of(playTable_.begin(), playTable_.end(), [kHPlay](const std::unique_ptr<PlayData>& data)
			{
				return kHPlay == data->handle;
			});

		if (!kIsDuplicate)
			return kHPlay;
	}

	// ランダムでユニークなハンドルが見つからなかった場合、連番でユニークなハンドルを生成する
	for (uint32_t kHPlay = 1; kHPlay < std::numeric_limits<uint32_t>::max(); ++kHPlay)
	{
		const bool kIsDuplicate = std::any_of(playTable_.begin(), playTable_.end(), [kHPlay](const std::unique_ptr<PlayData>& data)
			{
				return kHPlay == data->handle;
			}
		);

		if (!kIsDuplicate)
			return kHPlay;
	}

	return 0;
}

/// @brief 音量を制御する
/// @param volume 
/// @return 
float Detail::AudioStore::ClampVolume(float volume)
{
	const float kMaxSoundVolume = 1.0f;
	const float kMinSoundVolume = 0.0f;
	return std::clamp(volume, kMinSoundVolume, kMaxSoundVolume);
}
