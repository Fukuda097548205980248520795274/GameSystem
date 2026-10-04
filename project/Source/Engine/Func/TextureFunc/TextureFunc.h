#pragma once
#include <vector>
#include <cstdint>
#include <wrl.h>
#include <d3d12.h>
#include <DirectXTex.h>
#include <d3dx12.h>

#include "Func/ConvertString/ConvertString.h"

namespace Detail
{
	/// @brief テクスチャを読み込みデータを取得する
	/// @param filePath 
	/// @return 
	DirectX::ScratchImage LoadTextureGetMipImages(const std::string& filePath);

	/// @brief テクスチャ用リソースを生成する
	/// @param device 
	/// @param metadata 
	/// @return 
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

	/// @brief テクスチャリソースをGPUに転送する命令をコマンドリストに登録する
	/// @param texture 
	/// @param mipImages 
	/// @param device 
	/// @param commandList 
	/// @return 
	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages,
		ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
}