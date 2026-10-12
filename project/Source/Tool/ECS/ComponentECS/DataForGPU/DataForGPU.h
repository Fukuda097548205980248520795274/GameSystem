#pragma once
#include "Matrix/Matrix4x4/Matrix4x4.h"

namespace Detail
{
	struct VertexMeshDataForGPU
	{
		/// @brief 位置
		Vector4 position;
		
		/// @brief テクスチャ座標
		Vector2 texcoord;
	};

	/// @brief トランスフォームデータをGPU用に格納する構造体
	struct TransformDataForGPU
	{
		// @brief コンストラクタ
		TransformDataForGPU() : world(), worldViewProjection(), worldInverseTranspose() {}

		/// @brief ワールド行列
		Matrix4x4 world;

		/// @brief ワールドビュープロジェクション行列
		Matrix4x4 worldViewProjection;

		/// @brief 逆転置行列
		Matrix4x4 worldInverseTranspose;
	};

	/// @brief ライティングなしのマテリアルデータをGPU用に格納する構造体
	struct UnLightingMaterialDataForGPU
	{
		UnLightingMaterialDataForGPU() : color(1.0f, 1.0f, 1.0f, 1.0f), uvTransform() {}

		/// @brief 色
		Vector4 color;

		/// @brief UV変換行列
		Matrix4x4 uvTransform;
	};
}