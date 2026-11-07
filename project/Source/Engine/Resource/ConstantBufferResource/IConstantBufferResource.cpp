#include "IConstantBufferResource.hpp"

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::IConstantBufferResource::RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
    commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, regions_[frameIndex].gpuAddress);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::IConstantBufferResource::RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootConstantBufferView(rootParameterIndex, regions_[frameIndex].gpuAddress);
}
