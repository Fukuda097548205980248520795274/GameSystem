#pragma once
#include "Store/ModelStore/ModelData.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace Detail
{
	class TextureStore;
	class DXHeap;

	/// @brief モデルを読み込む
	/// @param directory 
	/// @param fileName 
	/// @param textureStore 
	/// @param heap 
	/// @param device 
	/// @param commandList 
	/// @return 
	ModelData LoadModel(const std::string& directory, const std::string& fileName,
		TextureStore* textureStore, DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	/// @brief ノードを読み込む
	/// @param modelData 
	/// @param directory 
	/// @param fileName 
	void LoadNode(ModelData& modelData, const std::string& directory, const std::string& fileName);

	/// @brief ノードの再帰読み込み
	/// @param aiNode 
	/// @param parentIndex 
	/// @param modelData 
	void ReadNodeRecursive(const aiNode* aiNode, int32_t parentIndex, ModelData& modelData);

	/// @brief ノードの更新処理
	/// @param modelData 
	/// @param nodeIndex 
	void UpdateWorldMatrix(ModelData& modelData, int32_t nodeIndex);
}