#include "RWTexture2DBufferResource.h"
#include <cassert>

#include "Func/Barrier/Barrier.h"
#include "Engine.h"

Detail::RWTexture2DBufferResource::~RWTexture2DBufferResource()
{
	if (heap_)
	{
		for (int i = 0; i < resource_.size(); ++i)
		{
			// SRVハンドルを解放する
			heap_->FreeSrvDescriptorHandle(srvHandle_[i]);

			// UAVハンドルを解放する
			heap_->FreeSrvDescriptorHandle(uavHandle_[i]);
		}
	}
}

/// @brief 初期化
/// @param device 
/// @param commandList 
/// @param heap 
/// @param width 
/// @param height 
/// @param maxBufferCount 
void Detail::RWTexture2DBufferResource::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXHeap* heap,
	uint32_t width, uint32_t height, uint32_t maxBufferCount)
{
	// nullptrチェック
	assert(device);
	assert(heap);

	// 引数を受け取る
	width_ = width;
	height_ = height;
	heap_ = heap;

	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	// 最大バッファ数を取得する
	int bufferCount = 1;
	if (engine) bufferCount = maxBufferCount;

	// リソースとハンドルの配列をリサイズする
	resource_.resize(bufferCount);
	srvHandle_.resize(bufferCount);
	uavHandle_.resize(bufferCount);

	for (int i = 0; i < bufferCount; ++i)
	{
		// リソース作成
		resource_[i] = CreateUAVTextureResource(device, commandList, width_, height_);


		// UAVの設定
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		// ハンドルを取得する
		uavHandle_[i] = heap->GetSrvDescriptorHandle();

		// ビューの生成
		device->CreateUnorderedAccessView(resource_[i].Get(), nullptr, &uavDesc, uavHandle_[i].cpuHandle);


		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		// ハンドルを取得する
		srvHandle_[i] = heap->GetSrvDescriptorHandle();

		// ビューの生成
		device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
	}
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::RWTexture2DBufferResource::RegisterGraphicsSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::RWTexture2DBufferResource::RegisterGraphicsUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, uavHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::RWTexture2DBufferResource::RegisterComputeUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, uavHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::RWTexture2DBufferResource::RegisterComputeSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief バリアを張る
/// @param commandList 
/// @param before 
/// @param after 
void Detail::RWTexture2DBufferResource::Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex)
{
	TransitionBarrier(resource_[frameIndex].Get(), before, after, commandList);
}

/// @brief 全てのバリアを張る
/// @param commandList 
/// @param before 
/// @param after 
void Detail::RWTexture2DBufferResource::AllBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	for (int i = 0; i < resource_.size(); ++i)
	{
		TransitionBarrier(resource_[i].Get(), before, after, commandList);
	}
}