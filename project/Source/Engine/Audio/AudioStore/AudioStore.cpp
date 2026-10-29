#include "AudioStore.hpp"
#include <cassert>
#include <algorithm>
#include <limits>
#include "Func/ConvertString/ConvertString.hpp"
#include "Func/RandomFunc/RandomFunc.hpp"
#include <format>

#include "Engine.hpp"

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
	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	// MFの初期化（ローカル版）
	HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, std::format("MF初期化 : 0x{:X}", hr));
		throw std::runtime_error("Error : MFStartup");
	}
	if (engine)engine->Log(LogLevel::Info, "MF初期化");

	// XAudio2を初期化する
	hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, std::format("XAudio2初期化 : 0x{:X}", hr));
		throw std::runtime_error("Error : XAudio2Create");
	}
	if (engine)engine->Log(LogLevel::Info, "XAudio2初期化");

	// マスターボイスを生成する
	hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
	assert(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, std::format("マスターボイス生成 : 0x{:X}", hr));
		throw std::runtime_error("Error : CreateMasteringVoice");
	}
	if (engine)engine->Log(LogLevel::Info, "マスターボイス生成");
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
Detail::AudioHandle Detail::AudioStore::Load(const std::string& filePath)
{
	for (std::unique_ptr<AudioData>& data : audioTable_)
	{
		if (filePath == data->filePath)
			return data->handle;
	}

	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	if (!std::filesystem::exists(filePath))
	{
		if (engine) engine->Log(LogLevel::Error, "サウンドファイルが存在しません : " + filePath);
		return AudioHandle::Invalid(); // 無効なハンドルを返す
	}

	// ファイルのハッシュ値を計算する
	uint64_t fileHash = ComputeFileHash(filePath);
	std::filesystem::path srcPath(filePath);
	std::string extension = srcPath.extension().string();
	std::string stem = srcPath.stem().string();

	// 中間データのファイル名を作成する
	std::string cookedFileName = stem + "_" + std::to_string(fileHash) + extension;
	std::string cookedFilePath = kCookedDir + cookedFileName;

	// キャッシュが存在しない場合はコピーを実行
	if (!std::filesystem::exists(cookedFilePath))
	{
		if (!std::filesystem::exists(kCookedDir))
		{
			std::filesystem::create_directories(kCookedDir);
		}
		std::filesystem::copy_file(filePath, cookedFilePath, std::filesystem::copy_options::overwrite_existing);
		if (engine) engine->Log(LogLevel::Info, "サウンドの中間データを作成しました : " + cookedFilePath);
	}

	// MFソースリーダーを作成する
	const std::wstring kFilePathW = ConvertString(cookedFilePath);
	ComPtr<IMFSourceReader> pMFSourceReader{ nullptr };
	HRESULT hr = MFCreateSourceReaderFromURL(kFilePathW.c_str(), NULL, &pMFSourceReader);

	// アサーションを削除し、エラー時は早期リターン
	if (FAILED(hr))
	{
		if (engine) engine->Log(LogLevel::Error, "ソースリーダーの作成に失敗しました : " + filePath);
		return AudioHandle::Invalid();
	}

	ComPtr<IMFMediaType> pReader{ nullptr };
	hr = MFCreateMediaType(&pReader);
	if (FAILED(hr)) return AudioHandle::Invalid();

	pReader->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pReader->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	hr = pMFSourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pReader.Get());
	if (FAILED(hr)) return AudioHandle::Invalid();

	ComPtr<IMFMediaType> pOutType;
	hr = pMFSourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutType);
	if (FAILED(hr)) return AudioHandle::Invalid();

	std::unique_ptr<AudioData> audioDatum = std::make_unique<AudioData>();
	audioDatum->filePath = filePath;

	// 構造体ハンドルを生成 (generation に 1 を設定して有効化)
	AudioHandle hAudio;
	hAudio.index = static_cast<uint32_t>(audioTable_.size());
	hAudio.generation = 1;
	audioDatum->handle = hAudio;

	MFCreateWaveFormatExFromMFMediaType(pOutType.Get(), &audioDatum->waveFormat, nullptr);
	if (audioDatum->waveFormat && audioDatum->waveFormat->wFormatTag == WAVE_FORMAT_PCM)
		audioDatum->waveFormat->cbSize = 0;

	while (true)
	{
		ComPtr<IMFSample> pMFSample{ nullptr };
		DWORD streamIndex = 0, flags = 0;
		LONGLONG llTimeStamp = 0;

		hr = pMFSourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &pMFSample);
		if (FAILED(hr)) break; // エラー時はそこまでのデータを残すか抜ける
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;

		if (pMFSample)
		{
			ComPtr<IMFMediaBuffer> pBuffer{ nullptr };
			hr = pMFSample->ConvertToContiguousBuffer(&pBuffer);
			if (FAILED(hr)) continue;

			BYTE* pData{ nullptr };
			DWORD currentLength = 0;
			hr = pBuffer->Lock(&pData, nullptr, &currentLength);
			if (FAILED(hr)) continue;

			audioDatum->mediaData.resize(audioDatum->mediaData.size() + currentLength);
			memcpy(audioDatum->mediaData.data() + audioDatum->mediaData.size() - currentLength, pData, currentLength);
			pBuffer->Unlock();
		}
	}

	audioTable_.push_back(std::move(audioDatum));
	return hAudio;
}


/// @brief オーディオを流す
/// @param handle 
/// @param volume 
/// @param isLoop 
/// @return 
Detail::PlayHandle Detail::AudioStore::PlayAudio(AudioHandle hAudio, float volume, bool isLoop)
{
	if (!xAudio2_ || !hAudio.IsValid()) return PlayHandle::Invalid();

	const AudioData* audioData = FindAudioData(hAudio);
	if (!audioData || !audioData->waveFormat || audioData->mediaData.empty())
		return PlayHandle::Invalid();

	std::unique_ptr<PlayData> playDatum = std::make_unique<PlayData>();
	PlayHandle hPlay = GenerateUniquePlayHandle();
	if (!hPlay.IsValid()) return PlayHandle::Invalid(); // 生成失敗時

	playDatum->handle = hPlay;

	IXAudio2SourceVoice* rawVoice = nullptr;
	HRESULT hr = xAudio2_->CreateSourceVoice(&rawVoice, audioData->waveFormat);
	if (FAILED(hr) || !rawVoice) return PlayHandle::Invalid();

	playDatum->pSourceVoice.reset(rawVoice);

	XAUDIO2_BUFFER buffer{ 0 };
	buffer.pAudioData = audioData->mediaData.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = sizeof(BYTE) * static_cast<UINT32>(audioData->mediaData.size());
	if (isLoop) buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

	hr = playDatum->pSourceVoice->SubmitSourceBuffer(&buffer);
	if (FAILED(hr)) return PlayHandle::Invalid();

	volume = ClampVolume(volume);
	hr = playDatum->pSourceVoice->SetVolume(volume);
	if (FAILED(hr)) return PlayHandle::Invalid();

	hr = playDatum->pSourceVoice->Start(0);
	if (FAILED(hr)) return PlayHandle::Invalid();

	playTable_.push_back(std::move(playDatum));
	return hPlay;
}


/// @brief オーディオを停止する
/// @param handle 
void Detail::AudioStore::StopAudio(PlayHandle hPlay)
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
bool Detail::AudioStore::IsAudioPlay(PlayHandle hPlay)
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
void Detail::AudioStore::SetVolume(PlayHandle hPlay, float volume)
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
void Detail::AudioStore::SetPitch(PlayHandle hPlay, float pitch)
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
Detail::PlayData* Detail::AudioStore::FindPlayData(PlayHandle hPlay)
{
	if (!hPlay.IsValid()) return nullptr;
	for (auto& playDatum : playTable_)
	{
		if (hPlay == playDatum->handle) return playDatum.get();
	}
	return nullptr;
}

/// @brief ハンドルに対応するオーディオデータを探す
/// @param handle 
/// @return 
const Detail::AudioData* Detail::AudioStore::FindAudioData(AudioHandle hAudio) const
{
	if (!hAudio.IsValid() || hAudio.index >= audioTable_.size()) return nullptr;
	return audioTable_[hAudio.index].get();
}

/// @brief ユニークなプレイハンドルを生成する
/// @return 
Detail::PlayHandle Detail::AudioStore::GenerateUniquePlayHandle() const
{
	constexpr uint32_t kMaxRetryCount = 64;
	for (uint32_t retry = 0; retry < kMaxRetryCount; ++retry)
	{
		const uint32_t index = GetRandomRange(1, 10000000);
		const bool isDuplicate = std::any_of(playTable_.begin(), playTable_.end(), [index](const std::unique_ptr<PlayData>& data) {
			return index == data->handle.index;
			});

		if (!isDuplicate) return PlayHandle{ index, 1 };
	}
	return PlayHandle::Invalid();
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

/// @brief パスを正規化する
/// @param path 
/// @return 
std::string Detail::AudioStore::NormalizePath(const std::string& path) const
{
	std::filesystem::path p(path);
	std::string normalized = p.lexically_normal().string();
	std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return normalized;
}

/// @brief ファイルのハッシュ値を計算する
/// @param filePath
/// @return
uint64_t Detail::AudioStore::ComputeFileHash(const std::string& filePath) const
{
	std::ifstream file(filePath, std::ios::binary);
	if (!file) return 0;

	uint64_t hash = 14695981039346656037ULL; // FNV-1a初期値
	char buffer[4096];
	while (file.read(buffer, sizeof(buffer)))
	{
		std::streamsize bytes = file.gcount();
		for (std::streamsize i = 0; i < bytes; ++i)
		{
			hash ^= static_cast<uint8_t>(buffer[i]);
			hash *= 1099511628211ULL;
		}
	}

	std::streamsize bytes = file.gcount();
	for (std::streamsize i = 0; i < bytes; ++i) {
		hash ^= static_cast<uint8_t>(buffer[i]);
		hash *= 1099511628211ULL;
	}

	return hash;
}