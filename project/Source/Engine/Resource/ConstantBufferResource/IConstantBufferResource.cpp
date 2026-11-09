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

/// @brief リソースを取得する
/// @param bufferIndex 
/// @return 
ID3D12Resource* Detail::IConstantBufferResource::Get(uint32_t bufferIndex) const
{
    // バッファインデックスが範囲内であることを確認する
    assert(bufferIndex < regions_.size());

    // リソースを返す
    return regions_[bufferIndex].parentBuffer->GetResource();
}
