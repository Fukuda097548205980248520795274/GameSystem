#include "IStructuredBufferResource.hpp"

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

/// @brief リソースを取得する
/// @param bufferIndex 
/// @return 
ID3D12Resource* Detail::IStructuredBufferResource::Get(uint32_t bufferIndex) const
{
    // バッファインデックスが範囲内であることを確認する
    assert(bufferIndex < regions_.size());

    // サブアロケーションされた領域の親バッファからリソースを取得する
    return regions_[bufferIndex].parentBuffer->GetResource();
}
