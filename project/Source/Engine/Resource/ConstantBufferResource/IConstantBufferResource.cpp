#include "IConstantBufferResource.h"

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::IConstantBufferResource::RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, resource_[frameIndex]->GetGPUVirtualAddress());
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::IConstantBufferResource::RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootConstantBufferView(rootParameterIndex, resource_[frameIndex]->GetGPUVirtualAddress());
}