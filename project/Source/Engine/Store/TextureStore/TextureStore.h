#pragma once
#include "RenderContext/DXHeap/DXHeap.h"
#include "ECS/ComponentECS/ComponentECS.h"
#include <DirectXTex.h>
#include <d3dx12.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

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

	class TextureStore
	{
	public:

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
		};


	public:

		/// @brief テクスチャを読み込む
		/// @param filePath 
		/// @param heap 
		/// @param device 
		/// @param commandList 
		/// @return 
		uint32_t Load(const std::string& filePath, DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

		/// @brief ファイルパスを取得する
		/// @param handle 
		/// @return 
		std::string GetFilePath(uint32_t handle) const;

		/// @brief SRV用GPUハンドルを取得する
		/// @param handle 
		/// @return 
		SRVDescriptorHandle GetSrvHandle(uint32_t handle) const;

		/// @brief テクスチャの幅を取得する
		/// @param handle 
		/// @return 
		size_t GetTextureWidth(uint32_t handle) const;

		/// @brief テクスチャの高さを取得する
		/// @param handle 
		/// @return 
		size_t GetTextureHeight(uint32_t handle) const;

		/// @brief ハンドルを取得する
		/// @param filePath 
		/// @return 
		uint32_t GetHandle(const std::string& filePath) const;

		/// @brief テクスチャの種類を取得する
		/// @param handle 
		/// @return 
		TextureType GetType(uint32_t handle) const;

		/// @brief 中間リソースを解放する
		void ReleaseIntermediateResources();


	private:

		/// @brief パスを正規化する
		/// @param path 
		/// @return 
		std::string NormalizePath(const std::string& path) const;

	private:

		/// @brief テクスチャデータのテーブル
		std::vector<std::unique_ptr<TextureData>> dataTable_;

		/// @brief ファイルパスとハンドルのマップ
		std::unordered_map<std::string, uint32_t> pathToHandleMap_;

		/// @brief 中間リソースの解放待ちリスト
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingUploadResources_;
	};
}