#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>
#include "Vector/Vector4/Vector4.h"

namespace Detail
{
	/// @brief バッファリソースを生成する
	/// @param device 
	/// @param sizeInBytes 
	/// @return 
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

	/// @brief 書き込み可能なテクスチャを生成する
	/// @param device 
	/// @param width 
	/// @param height 
	/// @param clearColor 
	/// @return 
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(ID3D12Device* device, uint32_t width, uint32_t height,const Vector4& clearColor);

	/// @brief 深度テクスチャリソースを生成する
	/// @param device 
	/// @param width 
	/// @param height 
	/// @return 
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);
}