#include "ResourceFunc.h"
#include <cassert>
#include <stdexcept>

/// @brief バッファリソースを生成する
/// @param device 
/// @param sizeInBytes 
/// @return 
Microsoft::WRL::ComPtr<ID3D12Resource> Detail::CreateBufferResource(ID3D12Device* device, size_t sizeInBytes)
{
	// ヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));

	if (!SUCCEEDED(hr))
	{
		throw std::runtime_error("Failed to create buffer resource.");
	}


	return resource;
}

/// @brief 書き込み可能なテクスチャを生成する
/// @param device 
/// @param width 
/// @param height 
/// @param clearColor 
/// @return 
Microsoft::WRL::ComPtr<ID3D12Resource> Detail::CreateRenderTextureResource(ID3D12Device* device, uint32_t width, uint32_t height,const Vector4& clearColor)
{
	/*-----------------------
		リソースの設定を行う
	-----------------------*/

	D3D12_RESOURCE_DESC resourceDesc{};

	resourceDesc.Width = UINT(width);
	resourceDesc.Height = UINT(height);
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// 書き込める設定
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;


	/*----------------------
		ヒープの設定を行う
	----------------------*/

	D3D12_HEAP_PROPERTIES heapProperties{};

	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;


	/*----------------------
		クリア最適値の設定
	----------------------*/

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	clearValue.Color[0] = clearColor.x;
	clearValue.Color[1] = clearColor.y;
	clearValue.Color[2] = clearColor.z;
	clearValue.Color[3] = clearColor.w;


	/*-------------------
		リソースの生成
	-------------------*/

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&resource)
	);

	if (!SUCCEEDED(hr))
	{
		throw std::runtime_error("Failed to create render texture resource.");
	}

	return resource;
}

/// @brief 深度テクスチャリソースを生成する
/// @param device 
/// @param width 
/// @param height 
/// @return 
Microsoft::WRL::ComPtr<ID3D12Resource> Detail::CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height)
{
	/*------------------
		リソースの設定
	------------------*/

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;


	/*----------------
		ヒープの設定
	----------------*/

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;


	/*----------------------
		深度値のクリア設定
	----------------------*/

	D3D12_CLEAR_VALUE depthClearValue{};

	// 1.0fでクリアする
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;


	/*----------------------
		リソースを生成する
	----------------------*/

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		// ヒープの設定
		&heapProperties,

		// ヒープの特殊な設定
		D3D12_HEAP_FLAG_NONE,

		// リソースの設定
		&resourceDesc,

		// 深度値を書き込む設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,

		// クリア最適地
		&depthClearValue,

		IID_PPV_ARGS(&resource)
	);

	if (!SUCCEEDED(hr))
	{
		throw std::runtime_error("Failed to create depth stencil texture resource.");
	}

	return resource;
}