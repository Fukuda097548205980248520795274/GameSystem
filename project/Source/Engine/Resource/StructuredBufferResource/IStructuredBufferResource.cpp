#include "IStructuredBufferResource.h"

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::IStructuredBufferResource::RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, handle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::IStructuredBufferResource::RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, handle_[frameIndex].gpuHandle);
}