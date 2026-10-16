#include "IRWStructuredBufferResource.h"

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::IRWStructuredBufferResource::RegisterComputeUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, uavHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::IRWStructuredBufferResource::RegisterComputeSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::IRWStructuredBufferResource::RegisterGraphicsSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief バリアを張る
/// @param commandList 
/// @param before 
/// @param after 
void Detail::IRWStructuredBufferResource::Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex)
{
	TransitionBarrier(resource_[frameIndex].Get(), before, after, commandList);
}

/// @brief 全てのバリアを張る
/// @param commandList 
/// @param before 
/// @param after 
void Detail::IRWStructuredBufferResource::AllBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	for (int i = 0; i < resource_.size(); ++i)
	{
		TransitionBarrier(resource_[i].Get(), before, after, commandList);
	}
}