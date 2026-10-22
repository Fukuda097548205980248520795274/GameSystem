#pragma once
#include "Matrix/Matrix4x4/Matrix4x4.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>

#include "Resource/VertexBufferResource/VertexBufferResource.h"
#include "Resource/IndexBufferResource/IndexBufferResource.h"

#include "Store/TextureStore/TextureData.h"

namespace Detail
{
	/// @brief マテリアルデータ
	struct MaterialData
	{
		// テクスチャハンドル
		Detail::TextureHandle handle;
	};

	/// @brief モデル用頂点データ
	struct ModelVertexData
	{
		// ローカル座標
		Vector4 position;

		// UV座標
		Vector2 texcoord;

		// 法線
		Vector3 normal;
	};

	// クォータニオンの3Dトランスフォーム
	struct QuaternionTransform3D
	{
		// 拡縮
		Vector3 scale;

		// 回転
		Quaternion rotate;

		// 移動
		Vector3 translate;
	};

	/// @brief モデルノード
	struct ModelNode
	{
		// 名前
		std::string name;

		// ローカル行列
		Matrix4x4 localMatrix;

		// ワールド行列
		Matrix4x4 worldMatrix;

		// クォータニオン用のトランスフォーム
		QuaternionTransform3D transform;

		// メッシュインデックス
		std::optional<int32_t> meshIndex;

		// 子ノードインデックス
		std::vector<int32_t> children;

		// 親ノードインデックス
		std::optional<int32_t> parent;
	};

	/// @brief メッシュデータ
	struct MeshData
	{
		// マテリアルデータ
		MaterialData material;


		// 頂点バッファ
		std::unique_ptr<VertexBufferResource<ModelVertexData>> vertexBufferResource;

		// インデックスバッファ
		std::unique_ptr<IndexBufferResource> indexBufferResource;
	};

	/// @brief モデルデータ
	struct ModelData
	{
		// モデルハンドル
		uint32_t handle;

		/// @brief ファイルパス
		std::string filePath;

		// メッシュデータ
		std::vector<MeshData> meshes;

		// メッシュインデックス
		std::unordered_map<std::string, int32_t> meshIndices;

		// メッシュ名
		std::vector<std::string> meshNames;

		// ノード
		std::vector<ModelNode> nodes;
	};

	/// @brief モデルハンドル
	struct ModelHandle
	{
		// インデックス
		uint32_t index = 0;

		// 世代番号
		uint32_t generation = 0;

		/// @brief 有効なハンドルかどうかを判定する
		/// @return
		bool IsValid() const { return generation != 0; }

		// 比較演算子
		bool operator==(const ModelHandle& other) const { return index == other.index && generation == other.generation; }
		bool operator!=(const ModelHandle& other) const { return !(*this == other); }

		/// @brief 無効なハンドルを取得する
		/// @return 
		static ModelHandle Invalid() { return ModelHandle{ 0, 0 }; }
	};
}