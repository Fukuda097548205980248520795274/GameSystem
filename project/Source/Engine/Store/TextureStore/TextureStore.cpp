#include "TextureStore.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cassert>
#include "Engine.h"

#include "Func/TextureFunc/TextureFunc.h"

/// @brief 初期化
/// @param device 
/// @param heap 
/// @param commandList 
void Detail::TextureStore::Initialize(ID3D12Device* device, DXHeap* heap, ID3D12GraphicsCommandList* commandList)
{
	// 初期テクスチャを読み込む
	defaultTextureHandle_ = Load("./Assets/Internal/Texture/white2x2.dds", heap, device, commandList);
	defaultCubemapHandle_ = Load("./Assets/Internal/Texture/white_skybox.dds", heap, device, commandList);
}

/// @brief パスを正規化する
/// @param path 
/// @return 
std::string Detail::TextureStore::NormalizePath(const std::string& path) const
{
	// パスを正規化する
	std::filesystem::path p(path);
	std::string normalized = p.lexically_normal().string();
	std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	return normalized;
}

/// @brief テクスチャのハッシュ値を計算する
/// @param image 
/// @return 
uint64_t Detail::TextureStore::ComputeImageHash(const DirectX::ScratchImage& image) const
{
	// FNV-1aハッシュアルゴリズムの初期値
	uint64_t hash = 14695981039346656037ULL;

	// DirectX::ScratchImageからピクセルデータを取得してハッシュ値を計算する
	const DirectX::Image* images = image.GetImages();
	size_t imageCount = image.GetImageCount();

	// すべてのスライス（ミップマップを含む）に対してハッシュ値を計算
	for (size_t i = 0; i < imageCount; ++i)
	{
		// スライスのピクセルデータとサイズを取得
		const uint8_t* pixels = images[i].pixels;
		size_t size = images[i].slicePitch;

		for (size_t j = 0; j < size; ++j)
		{
			hash ^= pixels[j];
			hash *= 1099511628211ULL;
		}
	}
	return hash;
}

/// @brief 更新処理
/// @param maxBufferCount 
/// @param frameIndex 
void Detail::TextureStore::Update(uint32_t maxBufferCount, uint32_t frameIndex)
{
	// アップロード待ちリソースの解放
	ProcessDeferredDeletions(maxBufferCount, frameIndex);
}

/// @brief テクスチャを読み込む
/// @param filePath 
/// @param heap 
/// @param device 
/// @param commandList 
/// @return 
Detail::TextureHandle Detail::TextureStore::Load(const std::string& filePath, DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	assert(heap && device && commandList);
	auto engine = Engine::GetInstance();

	// 既に読み込み済みか検索
	std::string normPath = NormalizePath(filePath);
	auto it = pathToHandleMap_.find(normPath);
	if (it != pathToHandleMap_.end())
	{
		// ハンドルが今も有効かチェックして返す
		if (GetTextureData(it->second) != nullptr)
			return it->second;
	}

	// 画像読み込み
	DirectX::ScratchImage image = LoadTextureGetMipImages(filePath);
	if (image.GetImageCount() == 0)
	{
		if (engine) engine->Log(LogLevel::Error, "テクスチャの読み込みに失敗 : " + filePath);
		return TextureHandle::Invalid();
	}

	// ハッシュ値の確認
	uint64_t hash = ComputeImageHash(image);
	auto hashIt = hashToHandleMap_.find(hash);
	if (hashIt != hashToHandleMap_.end())
	{
		if (GetTextureData(hashIt->second) != nullptr)
		{
			TextureHandle existingHandle = hashIt->second;
			pathToHandleMap_[normPath] = existingHandle;
			return existingHandle;
		}
	}

	// テクスチャリソースの作成
	auto textureData = std::make_unique<TextureData>();
	textureData->name = filePath;
	textureData->metadata = image.GetMetadata();
	textureData->hash = hash;

	const DirectX::TexMetadata& metadata = image.GetMetadata();
	textureData->type_ = metadata.IsCubemap() ? TextureType::Cubemap : TextureType::Texture2D;

	textureData->resource = CreateTextureResource(device, metadata);
	if (!textureData->resource)
	{
		if (engine) engine->Log(LogLevel::Error, "テクスチャリソースの生成に失敗 : " + filePath);
		return TextureHandle::Invalid();
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> subResource = UploadTextureData(textureData->resource.Get(), image, device, commandList);
	if (subResource)
	{
		int maxBufferCount = engine ? static_cast<int>(engine->GetMaxBufferCount()) + 1 : 3;
		pendingUploadResources_.push_back({ std::move(subResource), maxBufferCount });
	}

	// SRV作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (metadata.IsCubemap())
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);
	}

	textureData->srvHandle = heap->GetSrvDescriptorHandle();
	device->CreateShaderResourceView(textureData->resource.Get(), &srvDesc, textureData->srvHandle.cpuHandle);

	// スロットの確保
	uint32_t slotIndex = 0;
	if (!freeSlots_.empty())
	{
		slotIndex = freeSlots_.front();
		freeSlots_.pop();
	}
	else
	{
		slotIndex = static_cast<uint32_t>(slots_.size());
		slots_.emplace_back();
	}

	Slot& slot = slots_[slotIndex];
	slot.data = std::move(textureData);
	slot.inUse = true;

	TextureHandle handle{ slotIndex, slot.generation };
	slot.data->handle = handle.index; // 内部保持用

	pathToHandleMap_[normPath] = handle;
	hashToHandleMap_[hash] = handle;

	return handle;
}

/// @brief ファイルパスを取得する
/// @param handle 
/// @return 
std::string Detail::TextureStore::GetFilePath(TextureHandle handle) const
{
	TextureData* data = GetTextureData(handle);
	return data ? data->name : "";
}

/// @brief ハンドルを取得する
/// @param filePath 
/// @return 
Detail::TextureHandle Detail::TextureStore::GetHandle(const std::string& filePath) const
{
	std::string normPath = NormalizePath(filePath);
	auto it = pathToHandleMap_.find(normPath);
	if (it != pathToHandleMap_.end())
	{
		return it->second;
	}

	// 見つからなかった場合は無効なハンドルを返す
	return TextureHandle::Invalid();
}

/// @brief SRV用GPUハンドルを取得する
/// @param handle 
/// @return 
Detail::SRVDescriptorHandle Detail::TextureStore::GetSrvHandle(TextureHandle handle) const
{
	// ハンドルが有効かチェック
	TextureData* data = GetTextureData(handle);
	if (data)return data->srvHandle;

	// デフォルトテクスチャのSRVハンドルを返す
	TextureData* defaultData = GetTextureData(defaultTextureHandle_);
	if (defaultData)return defaultData->srvHandle;

	// デフォルトテクスチャが存在しない場合はNullハンドルを返す
	static SRVDescriptorHandle nullHandle{};
	return nullHandle;
}

/// @brief テクスチャの幅を取得する
/// @param handle 
/// @return 
size_t Detail::TextureStore::GetTextureWidth(TextureHandle handle) const
{
	TextureData* data = GetTextureData(handle);
	return data ? data->metadata.width : 0;
}

/// @brief テクスチャの高さを取得する
/// @param handle 
/// @return 
size_t Detail::TextureStore::GetTextureHeight(TextureHandle handle) const
{
	TextureData* data = GetTextureData(handle);
	return data ? data->metadata.height : 0;
}

/// @brief テクスチャの種類を取得する
/// @param handle 
/// @return 
Detail::TextureType Detail::TextureStore::GetType(TextureHandle handle) const
{
	TextureData* data = GetTextureData(handle);
	return data ? data->type_ : TextureType::Texture2D;
}

/// @brief テクスチャデータを取得する
/// @param handle 
/// @return 
Detail::TextureData* Detail::TextureStore::GetTextureData(TextureHandle handle) const
{
	// ハンドルが有効かチェック
	if (!handle.IsValid() || handle.index >= slots_.size()) return nullptr;

	// スロットの状態を確認
	const Slot& slot = slots_[handle.index];
	if (!slot.inUse || slot.generation != handle.generation) return nullptr;

	// スロットが使用中で、世代番号が一致する場合はテクスチャデータを返す
	return slot.data.get();
}

/// @brief ハンドルのインデックスを取得する
/// @param index 
/// @return 
Detail::TextureHandle Detail::TextureStore::GetHandleByIndex(uint32_t index) const
{
	// インデックスが範囲外か、スロットが使用中でない場合は無効なハンドルを返す
	if (index >= slots_.size() || !slots_[index].inUse)
		return TextureHandle::Invalid();

	// スロットが使用中であれば、世代番号を含めてハンドルを返す
	return TextureHandle{ index, slots_[index].generation };
}

/// @brief テクスチャを削除する
/// @param handle 
/// @param device
/// @param heap
void Detail::TextureStore::Remove(TextureHandle handle, DXHeap* heap, uint32_t frameIndex)
{
	if (!handle.IsValid() || handle.index >= slots_.size()) return;

	Slot& slot = slots_[handle.index];

	if (slot.inUse && slot.generation == handle.generation)
	{
		if (auto engine = Engine::GetInstance())
		{
			// 削除対象のテクスチャを使用しているエンティティを検索し、デフォルトテクスチャに切り替える
			if (auto registry = engine->GetRegistryECS())
			{
				auto* textureArray = registry->GetComponentArray<TextureComponent>();
				if (textureArray)
				{
					for (Entity entity : textureArray->GetEntities())
					{
						auto texComp = registry->GetComponent<TextureComponent>(entity);
						if (texComp && texComp->handle == handle)
						{
							texComp->handle = defaultTextureHandle_;
						}
					}
				}
			}
		}

		if (slot.data)
		{
			std::string normPath = NormalizePath(slot.data->name);
			pathToHandleMap_.erase(normPath);
			hashToHandleMap_.erase(slot.data->hash);

			// 即座に破棄せず、所有権を破棄キューに移動する
			deferredDeletionQueue_.push({ std::move(slot.data), heap, frameIndex });
		}

		// スロットは空いたものとして即座に再利用可能にする
		slot.inUse = false;
		slot.generation++;
		freeSlots_.push(handle.index);
	}
}

/// @brief 中間リソースを解放する
void Detail::TextureStore::ReleaseIntermediateResources()
{
	// 残りフレーム数を減らし、0になったものだけをリストから削除する
	pendingUploadResources_.erase(
		std::remove_if(pendingUploadResources_.begin(), pendingUploadResources_.end(),
			[](auto& item) {
				item.second--;
				return item.second == 0;
			}),
		pendingUploadResources_.end()
	);
}

/// @brief 破棄を遅延処理する
/// @param maxBufferCount 
/// @param frameIndex 
void Detail::TextureStore::ProcessDeferredDeletions(uint32_t maxBufferCount, uint32_t frameIndex)
{
	while (!deferredDeletionQueue_.empty())
	{
		// キュー先頭の破棄要求フレームを取得
		uint32_t requestedFrame = deferredDeletionQueue_.front().requestedFrameIndex;

		// 破棄要求からGPUが十分に処理を進めたか判定
		if (frameIndex - requestedFrame >= maxBufferCount)
		{
			auto& item = deferredDeletionQueue_.front();

			// SRVハンドルを解放
			if (item.heap && item.data)
			{
				item.heap->FreeSrvDescriptorHandle(item.data->srvHandle);
			}

			// キューからポップすることでリソースが安全に解放される
			deferredDeletionQueue_.pop();
		}
		else
		{
			// 先頭要素がまだ破棄できないならループを抜ける
			break;
		}
	}
}