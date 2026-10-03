#include "DepthResource.h"

#include "Engine.h"
#include "Func/Barrier/Barrier.h"
#include "Func/ResourceFunc/ResourceFunc.h"

/// @brief 初期化
/// @param device 
/// @param width 
/// @param height 
/// @param heap 
/// @param log 
void Detail::DepthResource::Initialize(ID3D12Device* device, int32_t width, int32_t height, DXHeap* heap)
{
	// nullptrチェック
	assert(device);
	assert(heap);

	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	// 最大バッファ数を取得する
	uint32_t maxBufferCount = 1;
	if (engine) maxBufferCount = engine->GetMaxBufferCount();

	// リソースとハンドルの配列をリサイズする
	resource_.resize(maxBufferCount);
	dsvHandle_.resize(maxBufferCount);
	dsvReadOnlyHandle_.resize(maxBufferCount);
	srvHandle_.resize(maxBufferCount);

	for (uint32_t i = 0; i < maxBufferCount; ++i)
	{
		// デプスステンシル用のリソースを生成する
		resource_[i] = CreateDepthStencilTextureResource(device, width, height);

		/*---------------
			DSVの設定
		---------------*/

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

		// RTVハンドルを取得する
		dsvHandle_[i] = heap->GetDsvDescriptorHandle();

		// DSV生成
		device->CreateDepthStencilView(resource_[i].Get(), &dsvDesc, dsvHandle_[i]);

		// 読み取り専用のDSVを生成する
		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH; // 深度書き込みを禁止するフラグ
		dsvReadOnlyHandle_[i] = heap->GetDsvDescriptorHandle();
		device->CreateDepthStencilView(resource_[i].Get(), &dsvDesc, dsvReadOnlyHandle_[i]);


		/*---------------
			SRVの設定
		---------------*/

		// SRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		// SRV用CPUハンドルを取得する
		srvHandle_[i] = heap->GetSrvDescriptorHandle();

		// SRV生成
		device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
	}
}

/// @brief サイズを作り直す
/// @param device 
/// @param width 
/// @param height 
void Detail::DepthResource::Resize(ID3D12Device* device, int32_t width, int32_t height)
{
	assert(device);

	auto engine = Engine::GetInstance();
	uint32_t maxBufferCount = 1;
	if (engine) maxBufferCount = engine->GetMaxBufferCount();

	for (uint32_t i = 0; i < maxBufferCount; ++i)
	{
		// リソース開放
		resource_[i].Reset();

		// 新たなサイズで再生成
		resource_[i] = CreateDepthStencilTextureResource(device, width, height);

		// DSV 同じ設定
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

		// DSV再生成（取得済みのハンドルを再利用）
		device->CreateDepthStencilView(resource_[i].Get(), &dsvDesc, dsvHandle_[i]);

		// 読み取り専用のDSVを生成する
		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH; // 深度書き込みを禁止するフラグ
		device->CreateDepthStencilView(resource_[i].Get(), &dsvDesc, dsvReadOnlyHandle_[i]);

		// SRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		// SRV再生成（取得済みのハンドルを再利用）
		device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
	}
}

/// @brief バリアを張る
/// @param commandList 
/// @param before 
/// @param after 
/// @param frameIndex
void Detail::DepthResource::Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex)
{
	TransitionBarrier(resource_[frameIndex].Get(), before, after, commandList);
}

/// @brief デプスステンシルのクリア
/// @param commandList 
/// @param frameIndex
void Detail::DepthResource::ClearDepthStencil(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	// クリア
	commandList->ClearDepthStencilView(dsvHandle_[frameIndex], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

/// @brief コマンドリストに登録する
/// @param commandList
/// @param rootParameterIndex
/// @param frameIndex
void Detail::DepthResource::RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	assert(commandList);
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief SRVをコマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::DepthResource::RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	assert(commandList);
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}