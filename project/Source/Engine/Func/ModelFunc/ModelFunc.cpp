#include "ModelFunc.hpp"
#include <cassert>
#include <filesystem>
#include "Store/TextureStore/TextureStore.hpp"
#include "Store/ResourceStore/ResourceStore.hpp"

/// @brief モデルを読み込む
/// @param directory 
/// @param fileName 
/// @param textureStore 
/// @param heap 
/// @param device 
/// @param commandList 
/// @param allocator 
/// @return 
Detail::ModelData Detail::LoadModel(const std::string& filePath, TextureStore* textureStore,
    DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ResourceStore* resourceStore)
{
	// nullptrチェック
	assert(textureStore);
	assert(heap);
	assert(device);
	assert(commandList);
    assert(resourceStore);

    // バッファアロケータを取得する
    auto  uploadAllocator = resourceStore->GetUploadBufferAllocator();
    auto  defaultAllocator = resourceStore->GetDefaultBufferAllocator();
    if (uploadAllocator == nullptr || defaultAllocator == nullptr)
        return ModelData();


	// AssImpでファイルを開く
	Assimp::Importer importer;

	// ファイルパスを分解する
	std::filesystem::path path(filePath);
	std::string directory = path.parent_path().string();

	// シーンのデータ
	const aiScene* scene = importer.ReadFile(filePath.c_str(),aiProcess_FlipWindingOrder |aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices |aiProcess_RemoveRedundantMaterials |aiProcess_CalcTangentSpace |aiProcess_GenSmoothNormals);

	// 読み込みに失敗した場合は空のModelDataを返す
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || !scene->HasMeshes())
		return ModelData();

	// 頂点データ
	Vector4 position = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	Vector3 normal = Vector3(0.0f, 0.0f, 0.0f);
	Vector2 texcoord = Vector2(0.0f, 0.0f);

	// モデルデータ
	ModelData modelData;
	modelData.filePath = filePath;

	// メッシュの数を確保する
	modelData.meshes.resize(scene->mNumMeshes);
	modelData.meshNames.resize(scene->mNumMeshes);

	// アニメーションを持つかどうかを判定する
	modelData.hasAnimations = scene->HasAnimations() && scene->mNumAnimations > 0;

	// スケルトンを持つかどうかを判定する
	modelData.hasSkeleton = scene->HasMeshes() && scene->mNumMeshes > 0 && scene->mMeshes[0]->HasBones();

	// メッシュ
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		// メッシュのデータ
		aiMesh* mesh = scene->mMeshes[meshIndex];

		// メッシュデータ
		MeshData meshData;


		// 法線、テクスチャ座標がない場合は中止する
		if (!mesh->HasNormals())
			return ModelData();


		// メッシュの名前を取得する
		std::string meshName = mesh->mName.C_Str();


		// 頂点データを格納する配列を作成する
		std::vector<ModelVertexData> vertices(mesh->mNumVertices);
		
		// 頂点データを登録する
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
		{
			// 頂点データを取得する
			aiVector3D& position = mesh->mVertices[vertexIndex];
			aiVector3D& normal = mesh->mNormals[vertexIndex];
			aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

			// 値を取得する
			ModelVertexData vertex;
			vertex.position = Vector4(position.x, position.y, position.z, 1.0f);
			vertex.normal = Vector3(normal.x, normal.y, normal.z);

			// テクスチャ座標がある場合は取得する
			if (mesh->HasTextureCoords(0))
			{
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
				vertex.texcoord = Vector2(texcoord.x, texcoord.y);
			}
			else
			{
				vertex.texcoord = Vector2(0.0f, 0.0f);
			}

			// 右手から左手に変換
			vertex.position.x *= -1.0f;
			vertex.normal.x *= -1.0f;

			// データに登録
			vertices[vertexIndex] = vertex;
		}



		// インデックスデータ
		std::vector<uint32_t> indices(mesh->mNumFaces * 3);

		// インデックスデータを登録する
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
		{
			// フェイスのデータ
			aiFace& face = mesh->mFaces[faceIndex];

			// 三角形のみサポート
			assert(face.mNumIndices == 3);

			// インデックスを登録
			for (uint32_t i = 0; i < face.mNumIndices; ++i)
			{
				indices[faceIndex * 3 + i] = face.mIndices[i];
			}
		}


		// マテリアルインデックス
		uint32_t materialIndex = mesh->mMaterialIndex;

		// マテリアルデータ
		aiMaterial* material = scene->mMaterials[materialIndex];

		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0)
		{
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);

			// テクスチャファイルパス
			meshData.material.handle = textureStore->Load(directory + "/" + textureFilePath.C_Str(), heap, device, commandList);

			// テクスチャハンドルが無効な場合はデフォルトテクスチャを使用する
			if (!meshData.material.handle.IsValid())
				meshData.material.handle = textureStore->GetDefaultTextureHandle();
		} 
		else
		{
			meshData.material.handle = textureStore->GetDefaultTextureHandle();
		}


		// 登録する
		modelData.meshes[meshIndex] = std::move(meshData);
		modelData.meshNames[meshIndex] = meshName;
		modelData.meshIndices[meshName] = meshIndex;
        modelData.meshes[meshIndex].vertices = std::move(vertices);

        // 頂点データのサイズ
        uint64_t vertexDataSize = sizeof(ModelVertexData) * mesh->mNumVertices;

		// 頂点データを作成する
		modelData.meshes[meshIndex].vertexBufferResource = std::make_unique<VertexBufferResource<ModelVertexData>>(defaultAllocator, device, mesh->mNumVertices, 0);
        SubAllocatedRegion stagingVertexRegion = uploadAllocator->Allocate(vertexDataSize);
		std::memcpy(stagingVertexRegion.cpuAddress, modelData.meshes[meshIndex].vertices.data(), vertexDataSize);

        // 頂点データをコピーする
        const SubAllocatedRegion& destVertexRegion = modelData.meshes[meshIndex].vertexBufferResource->GetRegion(0);
        destVertexRegion.parentBuffer->Transition(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->CopyBufferRegion(destVertexRegion.parentBuffer->GetResource(), destVertexRegion.allocation.offset,
            stagingVertexRegion.parentBuffer->GetResource(), stagingVertexRegion.allocation.offset, vertexDataSize);
        destVertexRegion.parentBuffer->Transition(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);



        // インデックスデータのサイズ
        uint64_t indexDataSize = sizeof(uint32_t) * (mesh->mNumFaces * 3);

		// インデックスデータを作成する
		modelData.meshes[meshIndex].indexBufferResource = std::make_unique<IndexBufferResource>(defaultAllocator, device, mesh->mNumFaces * 3, 0);
        SubAllocatedRegion stagingIndexRegion = uploadAllocator->Allocate(indexDataSize);
        std::memcpy(stagingIndexRegion.cpuAddress, indices.data(), indexDataSize);

        // インデックスデータをコピーする
        const SubAllocatedRegion& destIndexRegion = modelData.meshes[meshIndex].indexBufferResource->GetRegion(0);
        destIndexRegion.parentBuffer->Transition(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->CopyBufferRegion(destIndexRegion.parentBuffer->GetResource(), destIndexRegion.allocation.offset,
            stagingIndexRegion.parentBuffer->GetResource(), stagingIndexRegion.allocation.offset, indexDataSize);
        destIndexRegion.parentBuffer->Transition(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);


        // アップロード用のバッファを解放する
        resourceStore->FreeUploadRegionDeferred(stagingVertexRegion, 0);
        resourceStore->FreeUploadRegionDeferred(stagingIndexRegion, 0);
	}

	// ノードを再帰的に読み込む
	ReadNodeRecursive(scene->mRootNode, -1, modelData);

	return modelData;
}

/// @brief ノードの再帰読み込み
/// @param aiNode 
/// @param parentIndex 
/// @param modelData 
void Detail::ReadNodeRecursive(const aiNode* aiNode, int32_t parentIndex, ModelData& modelData)
{
	// 新しいノードを追加
	int32_t currentIndex = static_cast<int32_t>(modelData.nodes.size());
	modelData.nodes.emplace_back();
	ModelNode& node = modelData.nodes.back();



	// トランスフォームに格納する
	aiVector3D scale, translate;
	aiQuaternion rotate;
	aiNode->mTransformation.Decompose(scale, rotate, translate);

	node.transform.scale = Vector3(scale.x, scale.y, scale.z);

	// 右手 -> 左手
	node.transform.translate = Vector3(-translate.x, translate.y, translate.z);

	// X軸を反転　回転方向が逆なので軸を反転させる
	node.transform.rotate = Quaternion(rotate.x, -rotate.y, -rotate.z, rotate.w);



	// ノードの名前
	node.name = aiNode->mName.C_Str();

	// ローカル行列
	node.localMatrix = Make3DAffineMatrix4x4(node.transform.scale, node.transform.rotate, node.transform.translate);


	// 親インデックス
	if (parentIndex >= 0) 
	{
		node.parent = parentIndex;
		modelData.nodes[parentIndex].children.push_back(currentIndex);
	}

	// メッシュインデックス（複数ある場合は最初だけ対応）
	if (aiNode->mNumMeshes > 0) 
	{
		node.meshIndex = aiNode->mMeshes[0];
	}

	// 子ノードを再帰的に処理
	for (uint32_t i = 0; i < aiNode->mNumChildren; ++i) 
	{
		ReadNodeRecursive(aiNode->mChildren[i], currentIndex, modelData);
	}
}

/// @brief ノードの更新処理
/// @param modelData 
/// @param nodeIndex 
void Detail::UpdateWorldMatrix(ModelData& modelData, int32_t nodeIndex)
{
	ModelNode& node = modelData.nodes[nodeIndex];

	// 親ノードがある場合は親のワールド行列を掛ける
	if (node.parent.has_value())
	{
		node.worldMatrix = modelData.nodes[*node.parent].worldMatrix * node.localMatrix;
	} 
	else
	{
		node.worldMatrix = node.localMatrix;
	}

	// 自身の子ノードを更新
	for (int32_t child : node.children)
	{
		UpdateWorldMatrix(modelData, child);
	}
}
