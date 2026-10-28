#pragma once
#include <DirectXTex.h>
#include <d3dx12.h>

#include "Store/HandleData/HandleData.h"
#include "RenderContext/DXHeap/DXHeap.h"

namespace Detail
{
	/// @brief テクスチャの種類
	enum class TextureType
	{
		Texture2D,
		Cubemap
	};

	/// @brief 無効なテクスチャハンドル
	constexpr uint32_t kInvalidTextureHandle = UINT32_MAX;

	/// @brief テクスチャデータ
	struct TextureData
	{
		/// @brief リソース
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;

		// ミップマップ付きテクスチャデータ
		DirectX::TexMetadata metadata{};

		/// @brief SRVハンドル
		SRVDescriptorHandle srvHandle;

		// テクスチャのハンドル（またはID/ポインタ）
		uint32_t handle = kInvalidTextureHandle;

		// 名前
		std::string name{};

		/// @brief 種類
		TextureType type_;

		/// @brief ハッシュ値
		uint64_t hash = 0;
	};


	/// @brief テクスチャのタグ
	struct TextureTag {};

	/// @brief テクスチャハンドル
	using TextureHandle = Handle<TextureTag>;
}