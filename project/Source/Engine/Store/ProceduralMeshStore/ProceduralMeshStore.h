#pragma once
#include <unordered_map>
#include <memory>

#include "ECS/ComponentECS/DataForGPU/DataForGPU.h"
#include "Resource/VertexBufferResource/VertexBufferResource.h"
#include "Resource/IndexBufferResource/IndexBufferResource.h"

namespace Detail
{
	// プロシージャルメッシュの種類
	enum class ProceduralMeshType
	{
		Cube,
	};

	// メッシュデータの構造体
	struct ProceduralMeshData
	{
		std::unique_ptr<VertexBufferResource<Detail::VertexMeshDataForGPU>> vertexBuffer;
		std::unique_ptr<IndexBufferResource> indexBuffer;
	};

	class ProceduralMeshStore
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		ProceduralMeshStore(ID3D12Device* device) { Initialize(device); }

		/// @brief デストラクタ
		~ProceduralMeshStore() = default;

		/// @brief コマンドリストに登録する
		/// @param type 
		/// @param commandList 
		void Register(ProceduralMeshType type, ID3D12GraphicsCommandList* commandList);

		/// @brief 頂点数を取得する
		/// @param type 
		/// @return 
		UINT GetNumVertex(ProceduralMeshType type) const;

		/// @brief インデックス数を取得する
		/// @param type 
		/// @return 
		UINT GetNumIndex(ProceduralMeshType type) const;


	private:

		/// @brief 初期化
		/// @param device 
		void Initialize(ID3D12Device* device);

		/// @brief キューブメッシュを作成する
		/// @param device 
		void CreateCubeMesh(ID3D12Device* device);

		// メッシュデータのマップ
		std::unordered_map<ProceduralMeshType, ProceduralMeshData> meshDataMap_;

	};
}