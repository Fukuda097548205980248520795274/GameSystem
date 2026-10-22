#pragma once
#include <DirectXTex.h>
#include <d3dx12.h>

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

	/// @brief テクスチャハンドル
	struct TextureHandle
	{
		/// @brief インデックス
		uint32_t index = 0;

		/// @brief 世代番号
		uint32_t generation = 0;

		/// @brief 有効なハンドルかどうかを判定する
		/// @return 
		bool IsValid() const { return generation != 0; }

		// 比較演算子
		bool operator==(const TextureHandle& other) const { return index == other.index && generation == other.generation; }
		bool operator!=(const TextureHandle& other) const { return !(*this == other); }

		/// @brief 無効なハンドルを取得する
		/// @return 
		static TextureHandle Invalid() { return TextureHandle{ 0, 0 }; }
	};
}