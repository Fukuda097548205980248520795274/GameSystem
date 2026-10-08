#include "TextureStore.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cassert>
#include "Engine.h"

#include "Func/TextureFunc/TextureFunc.h"

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

/// @brief テクスチャを読み込む
/// @param filePath 
/// @param heap 
/// @param device 
/// @param commandList 
/// @return 
uint32_t Detail::TextureStore::Load(const std::string& filePath, DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	assert(heap);
	assert(device);
	assert(commandList);


	// Engineのインスタンスを取得する
	auto engine = Engine::GetInstance();

	// すでに読み込み済みかファイルパスマップから高速検索
	std::string normPath = NormalizePath(filePath);
	auto it = pathToHandleMap_.find(normPath);
	if (it != pathToHandleMap_.end())
	{
		return it->second;
	}

	// テクスチャを読み込む
	DirectX::ScratchImage image = LoadTextureGetMipImages(filePath);
	if (image.GetImageCount() == 0)
	{
		// 読み込みに失敗した場合は無効なハンドルを返す
		if (engine)engine->Log(LogLevel::Error, "テクスチャの読み込みに失敗 : " + filePath);
		return kInvalidTextureHandle;
	}
	if (engine)engine->Log(LogLevel::Info, "テクスチャを読み込みました : " + filePath);

	// ハッシュ値を計算して、すでに同じハッシュ値のテクスチャが存在するか確認
	uint64_t hash = ComputeImageHash(image);
	auto hashIt = hashToHandleMap_.find(hash);
	if (hashIt != hashToHandleMap_.end())
	{
		uint32_t existingHandle = hashIt->second;
		
		// すでに同じハッシュ値のテクスチャが存在する場合は、既存のハンドルを返す
		pathToHandleMap_[normPath] = existingHandle;
		return existingHandle;
	}

	// テクスチャデータを作成
	auto textureData = std::make_unique<TextureData>();
	textureData->name = filePath;
	textureData->metadata = image.GetMetadata();
	textureData->hash = hash;

	// メタデータからテクスチャの種類を判定
	const DirectX::TexMetadata& metadata = image.GetMetadata();
	textureData->type_ = metadata.IsCubemap() ? TextureType::Cubemap : TextureType::Texture2D;

	// テクスチャリソース・中間リソースの生成と転送
	textureData->resource = CreateTextureResource(device, metadata);
	if (!textureData->resource)
	{
		// 失敗した場合は無効なハンドルを返す
		if (engine)engine->Log(LogLevel::Error, "テクスチャリソースの生成に失敗 : " + filePath);
		return kInvalidTextureHandle;
	}
	if (engine)engine->Log(LogLevel::Info, "テクスチャリソースを生成しました : " + filePath);

	// 中間リソースを生成して転送コマンドを発行
	Microsoft::WRL::ComPtr<ID3D12Resource> subResource = UploadTextureData(textureData->resource.Get(), image, device, commandList);

	// 中間リソースは専用のリストに退避させる
	if (subResource)
	{
		int maxBufferCount = engine ? static_cast<int>(engine->GetMaxBufferCount()) + 1 : 3;
		pendingUploadResources_.push_back({ std::move(subResource), maxBufferCount });
	}

	// SRV設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// SRVの種類を設定（Cubemapか2Dテクスチャか）
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

	// CPU・GPUハンドル取得とSRV作成
	textureData->srvHandle = heap->GetSrvDescriptorHandle();
	device->CreateShaderResourceView(textureData->resource.Get(), &srvDesc, textureData->srvHandle.cpuHandle);

	// テーブルに登録
	uint32_t handle = static_cast<uint32_t>(dataTable_.size());
	textureData->handle = handle;

	dataTable_.push_back(std::move(textureData));
	pathToHandleMap_[normPath] = handle;
	hashToHandleMap_[hash] = handle;

	return handle;
}

/// @brief ファイルパスを取得する
/// @param handle 
/// @return 
std::string Detail::TextureStore::GetFilePath(uint32_t handle) const
{
	if (handle >= dataTable_.size() || !dataTable_[handle])
		return "";

	return dataTable_[handle]->name;
}

/// @brief ハンドルを取得する
/// @param filePath 
/// @return 
uint32_t Detail::TextureStore::GetHandle(const std::string& filePath) const
{
	std::string normPath = NormalizePath(filePath);
	auto it = pathToHandleMap_.find(normPath);
	if (it != pathToHandleMap_.end())
	{
		return it->second;
	}

	// 見つからなかった場合は無効なハンドルを返す
	return kInvalidTextureHandle;
}

/// @brief SRV用GPUハンドルを取得する
/// @param handle 
/// @return 
Detail::SRVDescriptorHandle Detail::TextureStore::GetSrvHandle(uint32_t handle) const
{
	// 範囲外アクセスや、すでに削除済みの場合は0番のハンドルを返す
	if (handle >= dataTable_.size() || !dataTable_[handle])
	{
		if (!dataTable_.empty() && dataTable_[0])
		{
			return dataTable_[0]->srvHandle;
		}

		// 0番も存在しない場合のフォールバック
		static SRVDescriptorHandle nullHandle{};
		return nullHandle;
	}

	return dataTable_[handle]->srvHandle;
}

/// @brief テクスチャの幅を取得する
/// @param handle 
/// @return 
size_t Detail::TextureStore::GetTextureWidth(uint32_t handle) const
{
	if (handle >= dataTable_.size() || !dataTable_[handle]) return 0;
	return dataTable_[handle]->metadata.width;
}

/// @brief テクスチャの高さを取得する
/// @param handle 
/// @return 
size_t Detail::TextureStore::GetTextureHeight(uint32_t handle) const
{
	if (handle >= dataTable_.size() || !dataTable_[handle]) return 0;
	return dataTable_[handle]->metadata.height;
}

/// @brief テクスチャの種類を取得する
/// @param handle 
/// @return 
Detail::TextureType Detail::TextureStore::GetType(uint32_t handle) const
{
	if (handle >= dataTable_.size() || !dataTable_[handle]) return TextureType::Texture2D;
	return dataTable_[handle]->type_;
}

/// @brief テクスチャを削除する
/// @param handle 
/// @param device
/// @param heap
void Detail::TextureStore::Remove(uint32_t handle, ID3D12Device* device, DXHeap* heap)
{
	// 範囲外アクセスや、すでに削除済みの場合は何もしない
	if (handle >= dataTable_.size() || !dataTable_[handle])
		return;

	// GPUの処理が完了するまで待機する
	if (auto engine = Engine::GetInstance())
		engine->WaitForGPU();

	// SRVをNullに置き換える
	if (device)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		// 0番のデフォルトテクスチャが存在する場合はそのリソースで上書き
		if (handle != 0 && !dataTable_.empty() && dataTable_[0] && dataTable_[0]->resource)
		{
			const auto& meta = dataTable_[0]->metadata;
			srvDesc.Format = meta.format;

			// SRVの種類を設定（Cubemapか2Dテクスチャか）
			if (meta.IsCubemap())
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MostDetailedMip = 0;
				srvDesc.TextureCube.MipLevels = UINT_MAX;
				srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = static_cast<UINT>(meta.mipLevels);
			}

			device->CreateShaderResourceView(dataTable_[0]->resource.Get(), &srvDesc, dataTable_[handle]->srvHandle.cpuHandle);
		}
		else
		{
			// デフォルトが存在しない場合は Null SRV で上書き
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;

			device->CreateShaderResourceView(nullptr, &srvDesc, dataTable_[handle]->srvHandle.cpuHandle);
		}
	}

	// ファイルパスマップから削除
	std::string normPath = NormalizePath(dataTable_[handle]->name);
	pathToHandleMap_.erase(normPath);
	hashToHandleMap_.erase(dataTable_[handle]->hash);

	// DXHeapからSRVディスクリプタを解放
	if (heap)heap->FreeSrvDescriptorHandle(dataTable_[handle]->srvHandle);

	// テーブルから削除
	dataTable_[handle].reset();
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
