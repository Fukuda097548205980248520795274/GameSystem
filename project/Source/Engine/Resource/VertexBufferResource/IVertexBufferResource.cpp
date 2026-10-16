#include "IVertexBufferResource.h"

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param frameIndex 
void Detail::IVertexBufferResource::Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	assert(commandList);

	// ビューをコマンドリストに設定する
	commandList->IASetVertexBuffers(0, 1, &view_[frameIndex]);
}