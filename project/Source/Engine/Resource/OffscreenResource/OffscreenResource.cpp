#include "OffscreenResource.h"
#include <cassert>
#include "Func/ResourceFunc/ResourceFunc.h"
#include "Func/Barrier/Barrier.h"
#include "Vector/Vector4/Vector4.h"

#include "Engine.h"

/// @brief 初期化
/// @param device 
/// @param heap 
/// @param width
/// @param height
void Detail::OffscreenResource::Initialize(ID3D12Device* device, DXHeap* heap, int32_t width, int32_t height)
{
	// nullptrチェック
	assert(device);
	assert(heap);


	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();
	
	// 最大バッファ数を取得する
	uint32_t maxBufferCount = 1;
	if (engine) maxBufferCount = engine->GetMaxBufferCount();

	// 配列のサイズを確保する
	resource_.resize(maxBufferCount);
	rtvHandle_.resize(maxBufferCount);
	srvHandle_.resize(maxBufferCount);

	for (uint32_t i = 0; i < maxBufferCount; ++i)
	{
		// 書き込み可能なリソーステクスチャを生成する
		resource_[i] = CreateRenderTextureResource(device, width, height, Vector4(0.0f, 0.0f, 0.0f, 0.0f));

		/*----------------
			RTVの設定
		----------------*/

		// スワップチェーンのRTV設定を反映させる
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		// ディスクリプタハンドルを取得する
		rtvHandle_[i] = heap->GetRtvDescriptorHandle();

		device->CreateRenderTargetView(resource_[i].Get(), &rtvDesc, rtvHandle_[i]);
		if(engine) engine->Log(LogLevel::Info, "RTV生成");



		/*---------------
			SRVの設定
		---------------*/

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.PlaneSlice = 0;

		// ディスクリプタハンドルを取得する
		srvHandle_[i] = heap->GetSrvDescriptorHandle();

		device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
		if (engine) engine->Log(LogLevel::Info, "SRV生成");
	}
}

/// @brief サイズを作り直す
/// @param device 
/// @param width
/// @param height
void Detail::OffscreenResource::Resize(ID3D12Device* device, int32_t width, int32_t height)
{
	assert(device);

	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	uint32_t maxBufferCount = 1;
	if (engine) maxBufferCount = engine->GetMaxBufferCount();

	for (uint32_t i = 0; i < maxBufferCount; ++i)
	{
		// リソース開放
		resource_[i].Reset();

		// 新たなサイズで作り直す
		resource_[i] = CreateRenderTextureResource(device, width, height, Vector4(0.0f, 0.0f, 0.0f, 0.0f));

		// スワップチェーンのRTV設定を反映させる
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		// RTV再生成（既存のハンドルを再利用）
		device->CreateRenderTargetView(resource_[i].Get(), &rtvDesc, rtvHandle_[i]);

		// SRV設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.PlaneSlice = 0;

		// SRV再生成（既存のハンドルを再利用）
		device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
	}
}

/// @brief バリアを張る
/// @param commandList 
/// @param before 
/// @param after 
/// @param frameIndex
void Detail::OffscreenResource::Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex)
{
	TransitionBarrier(resource_[frameIndex].Get(), before, after, commandList);
}

/// @brief 全てのバリアを張る
/// @param commandList 
/// @param before 
/// @param after 
void Detail::OffscreenResource::AllBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	for (auto& res : resource_)
	{
		TransitionBarrier(res.Get(), before, after, commandList);
	}
}

/// @brief レンダーターゲットの設定とクリア
/// @param commandList 
/// @param dsvHandle 
/// @param frameIndex
void Detail::OffscreenResource::ClearRenderTarget(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, uint32_t frameIndex)
{
	// nullptrチェック
	assert(commandList);

	// 設定
	commandList->OMSetRenderTargets(1, &rtvHandle_[frameIndex], false, &dsvHandle);

	// クリア
	float clearColor[] = { 0.0f, 0.0f ,0.0f, 0.0f };
	commandList->ClearRenderTargetView(rtvHandle_[frameIndex], clearColor, 0, nullptr);
}

/// @brief レンダーターゲットの設定
/// @param commandList 
/// @param dsvHandle 
/// @param frameIndex
void Detail::OffscreenResource::SetRenderTarget(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, uint32_t frameIndex)
{
	// nullptrチェック
	assert(commandList);

	// 設定
	commandList->OMSetRenderTargets(1, &rtvHandle_[frameIndex], false, &dsvHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::OffscreenResource::RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	// nullptrチェック
	assert(commandList);

	// テクスチャ
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::OffscreenResource::RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	// nullptrチェック
	assert(commandList);

	// テクスチャ
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}