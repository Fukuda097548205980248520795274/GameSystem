#pragma once
#include "Matrix/Matrix4x4/Matrix4x4.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

namespace Detail
{
	/// @brief マテリアルデータ
	struct MaterialData
	{
		// テクスチャハンドル
		uint32_t handle;
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

		// 子ノードインデックス
		std::vector<int32_t> children;

		// 親ノードインデックス
		std::optional<int32_t> parent;

		// メッシュインデックス
		std::optional<int32_t> meshIndex;
	};

	/// @brief メッシュデータ
	struct MeshData
	{
		// インデックスデータ
		std::vector<uint32_t> indices;

		// 頂点データ
		std::vector<ModelVertexData> vertices;

		// マテリアルデータ
		MaterialData material;
	};

	/// @brief モデルデータ
	struct ModelData
	{
		// メッシュデータ
		std::vector<MeshData> meshes;

		// メッシュインデックス
		std::unordered_map<std::string, int32_t> meshIndices;

		// メッシュ名
		std::vector<std::string> meshNames;

		// ノード
		std::vector<ModelNode> nodes;
	};

	/// @brief モデルストア
	class ModelStore
	{
	public:



	private:



	};
}