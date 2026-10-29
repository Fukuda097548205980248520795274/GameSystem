#include "ProceduralMeshStore.hpp"

/// @brief 初期化
/// @param device 
void Detail::ProceduralMeshStore::Initialize(ID3D12Device* device)
{
	// キューブメッシュを作成する
	CreateCubeMesh(device);
}

/// @brief コマンドリストに登録する
/// @param type 
/// @param commandList 
void Detail::ProceduralMeshStore::Register(ProceduralMeshType type, ID3D12GraphicsCommandList* commandList)
{
	auto it = meshDataMap_.find(type);
	if (it != meshDataMap_.end())
	{
		auto& meshData = it->second;
		meshData.vertexBuffer->Register(commandList, 0);
		meshData.indexBuffer->Register(commandList, 0);
	}
}

/// @brief 頂点数を取得する
/// @param type 
/// @return 
UINT Detail::ProceduralMeshStore::GetNumVertex(ProceduralMeshType type) const
{
	auto it = meshDataMap_.find(type);
	if (it != meshDataMap_.end())
	{
		return it->second.vertexBuffer->GetNumVertex();
	}
	return 0;
}

/// @brief インデックス数を取得する
/// @param type 
/// @return 
UINT Detail::ProceduralMeshStore::GetNumIndex(ProceduralMeshType type) const
{
	auto it = meshDataMap_.find(type);
	if (it != meshDataMap_.end())
	{
		return it->second.indexBuffer->GetNumIndex();
	}
	return 0;
}

/// @brief キューブメッシュを作成する
/// @param device 
void Detail::ProceduralMeshStore::CreateCubeMesh(ID3D12Device* device)
{
	// キューブメッシュの頂点データとインデックスデータを作成する
	meshDataMap_[ProceduralMeshType::Cube] = ProceduralMeshData
	{
		std::make_unique<VertexBufferResource<Detail::VertexMeshDataForGPU>>(device, 24, 1),
		std::make_unique<IndexBufferResource>(device, 36, 1)
	};

	auto& meshData = meshDataMap_[ProceduralMeshType::Cube];
	auto* vertexData = meshData.vertexBuffer->GetData(0);
	auto* indexData = meshData.indexBuffer->GetData(0);

	// 番号を与える
	for (int32_t i = 0; i < 6; ++i)
	{
		int32_t startIndex = i * 6;
		int32_t index = i * 4;

		indexData[startIndex] = index;
		indexData[startIndex + 1] = index + 1;
		indexData[startIndex + 2] = index + 2;
		indexData[startIndex + 3] = index + 1;
		indexData[startIndex + 4] = index + 3;
		indexData[startIndex + 5] = index + 2;
	}


	// ローカル座標を与える
	vertexData[0].position = Vector4(-1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[0].texcoord = Vector2(0.0f, 1.0f);
	vertexData[0].normal = Vector3(0.0f, 0.0f, -1.0f);
	vertexData[1].position = Vector4(-1.0f, 1.0f, -1.0f, 1.0f);
	vertexData[1].texcoord = Vector2(0.0f, 0.0f);
	vertexData[1].normal = Vector3(0.0f, 0.0f, -1.0f);
	vertexData[2].position = Vector4(1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[2].texcoord = Vector2(1.0f, 1.0f);
	vertexData[2].normal = Vector3(0.0f, 0.0f, -1.0f);
	vertexData[3].position = Vector4(1.0f, 1.0f, -1.0f, 1.0f);
	vertexData[3].texcoord = Vector2(1.0f, 0.0f);
	vertexData[3].normal = Vector3(0.0f, 0.0f, -1.0f);
	
	vertexData[4].position = Vector4(1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[4].texcoord = Vector2(0.0f, 1.0f);
	vertexData[4].normal = Vector3(1.0f, 0.0f, 0.0f);
	vertexData[5].position = Vector4(1.0f, 1.0f, -1.0f, 1.0f);
	vertexData[5].texcoord = Vector2(0.0f, 0.0f);
	vertexData[5].normal = Vector3(1.0f, 0.0f, 0.0f);
	vertexData[6].position = Vector4(1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[6].texcoord = Vector2(1.0f, 1.0f);
	vertexData[6].normal = Vector3(1.0f, 0.0f, 0.0f);
	vertexData[7].position = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	vertexData[7].texcoord = Vector2(1.0f, 0.0f);
	vertexData[7].normal = Vector3(1.0f, 0.0f, 0.0f);
	
	vertexData[8].position = Vector4(1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[8].texcoord = Vector2(0.0f, 1.0f);
	vertexData[8].normal = Vector3(0.0f, 0.0f, 1.0f);
	vertexData[9].position = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	vertexData[9].texcoord = Vector2(0.0f, 0.0f);
	vertexData[9].normal = Vector3(0.0f, 0.0f, 1.0f);
	vertexData[10].position = Vector4(-1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[10].texcoord = Vector2(1.0f, 1.0f);
	vertexData[10].normal = Vector3(0.0f, 0.0f, 1.0f);
	vertexData[11].position = Vector4(-1.0f, 1.0f, 1.0f, 1.0f);
	vertexData[11].texcoord = Vector2(1.0f, 0.0f);
	vertexData[11].normal = Vector3(0.0f, 0.0f, 1.0f);
	
	vertexData[12].position = Vector4(-1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[12].texcoord = Vector2(0.0f, 1.0f);
	vertexData[12].normal = Vector3(-1.0f, 0.0f, 0.0f);
	vertexData[13].position = Vector4(-1.0f, 1.0f, 1.0f, 1.0f);
	vertexData[13].texcoord = Vector2(0.0f, 0.0f);
	vertexData[13].normal = Vector3(-1.0f, 0.0f, 0.0f);
	vertexData[14].position = Vector4(-1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[14].texcoord = Vector2(1.0f, 1.0f);
	vertexData[14].normal = Vector3(-1.0f, 0.0f, 0.0f);
	vertexData[15].position = Vector4(-1.0f, 1.0f, -1.0f, 1.0f);
	vertexData[15].texcoord = Vector2(1.0f, 0.0f);
	vertexData[15].normal = Vector3(-1.0f, 0.0f, 0.0f);
	
	vertexData[16].position = Vector4(-1.0f, 1.0f, -1.0f, 1.0f);
	vertexData[16].texcoord = Vector2(0.0f, 1.0f);
	vertexData[16].normal = Vector3(0.0f, 1.0f, 0.0f);
	vertexData[17].position = Vector4(-1.0f, 1.0f, 1.0f, 1.0f);
	vertexData[17].texcoord = Vector2(0.0f, 0.0f);
	vertexData[17].normal = Vector3(0.0f, 1.0f, 0.0f);
	vertexData[18].position = Vector4(1.0f, 1.0f, -1.0f, 1.0f);
	vertexData[18].texcoord = Vector2(1.0f, 1.0f);
	vertexData[18].normal = Vector3(0.0f, 1.0f, 0.0f);
	vertexData[19].position = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	vertexData[19].texcoord = Vector2(1.0f, 0.0f);
	vertexData[19].normal = Vector3(0.0f, 1.0f, 0.0f);
	
	vertexData[20].position = Vector4(1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[20].texcoord = Vector2(0.0f, 1.0f);
	vertexData[20].normal = Vector3(0.0f, -1.0f, 0.0f);
	vertexData[21].position = Vector4(1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[21].texcoord = Vector2(0.0f, 0.0f);
	vertexData[21].normal = Vector3(0.0f, -1.0f, 0.0f);
	vertexData[22].position = Vector4(-1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[22].texcoord = Vector2(1.0f, 1.0f);
	vertexData[22].normal = Vector3(0.0f, -1.0f, 0.0f);
	vertexData[23].position = Vector4(-1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[23].texcoord = Vector2(1.0f, 0.0f);
	vertexData[23].normal = Vector3(0.0f, -1.0f, 0.0f);
}