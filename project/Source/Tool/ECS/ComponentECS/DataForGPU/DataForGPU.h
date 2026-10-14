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

	// @brief ライトカリング用のカメラデータをGPU用に格納する構造体
	struct LightCullingCameraDataForGPU
	{
		// @brief カメラの近クリップ平面
		float nearZ;

		// @brief カメラの遠クリップ平面
		float farZ;

		// @brief スクリーンの幅と高さ
		Vector2 screenDimensions;

		// @brief カメラのワールド座標
		Vector3 worldPosition;

		float padding0[1]; // パディング

		// @brief ビュー行列
		Matrix4x4 view;

		// @brief 射影行列
		Matrix4x4 projection;

		// @brief 逆射影行列
		Matrix4x4 inverseProjection;
	};

	// @brief ライトの数をGPU用に格納する構造体s
	struct NumLightDataForGPU
	{
		// @brief 平行光源の数
		int32_t numDirectionalLights;

		// @brief ポイントライトの数
		int32_t numPointLights;

		// @brief スポットライトの数
		int32_t numSpotLights;

		float padding[1]; // パディング
	};

	// @brief ポイントライトデータをGPU用に格納する構造体
	struct PointLightDataForGPU
	{
		// @brief 色
		Vector4 color;

		// @brief 位置
		Vector3 position;

		// @brief 光の強さ
		float intensity;

		// @brief 半径
		float radius;

		// @brief 減衰率
		float decay;
	};

	// @brief スポットライトデータをGPU用に格納する構造体
	struct SpotLightDataForGPU
	{
		// @brief 色
		Vector4 color;

		// @brief 位置
		Vector3 position;

		// @brief 光の強さ
		float intensity;

		// @brief 方向
		Vector3 direction;

		// @brief 半径
		float distance;

		// @brief 減衰率
		float decay;

		// @brief スポットライトの角度
		float cosAngle;

		// @brief 減衰開始角度のコサイン値
		float cosFalloffStart;
	};
}