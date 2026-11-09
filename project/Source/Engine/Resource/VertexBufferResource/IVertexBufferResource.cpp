#include "IVertexBufferResource.hpp"

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param frameIndex 
void Detail::IVertexBufferResource::Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	assert(commandList);

	// ビューをコマンドリストに設定する
	commandList->IASetVertexBuffers(0, 1, &view_[frameIndex]);
}

/// @brief リソースを取得する
/// @param bufferIndex 
/// @return 
ID3D12Resource* Detail::IVertexBufferResource::Get(uint32_t bufferIndex) const
{
    // バッファインデックスが範囲内であることを確認する
    assert(bufferIndex < regions_.size());

    // サブアロケーションされた領域の親バッファからリソースを取得する
    return regions_[bufferIndex].parentBuffer->GetResource();
}
