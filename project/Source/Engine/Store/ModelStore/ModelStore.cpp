#include "ModelStore.h"
#include "Func/ModelFunc/ModelFunc.h"

/// @brief 読み込み
/// @param directory 
/// @param fileName 
/// @param device 
/// @return 
uint32_t Detail::ModelStore::Load(const std::string& filePath, TextureStore* textureStore, DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	ModelData modelData = LoadModel(filePath, textureStore, heap, device, commandList);

	uint32_t handle = static_cast<uint32_t>(dataTable_.size());
	dataTable_.push_back(std::make_unique<ModelData>(std::move(modelData)));

	return handle;
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param modelHandle 
/// @param meshIndex 
void Detail::ModelStore::Register(ID3D12GraphicsCommandList* commandList, uint32_t modelHandle, int32_t meshIndex)
{
	// メッシュの頂点バッファをコマンドリストに登録する
	dataTable_[modelHandle]->meshes[meshIndex].vertexBufferResource->Register(commandList, 0);
	dataTable_[modelHandle]->meshes[meshIndex].indexBufferResource->Register(commandList, 0);
}