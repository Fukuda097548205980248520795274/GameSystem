#pragma once
#include "../../Vector/Vector2/Vector2.h"
#include "../../Vector/Vector3/Vector3.h"
#include "../../Vector/Vector4/Vector4.h"
#include "../../Quaternion/Quaternion.h"
#include <cstdint>

// 4x4行列
struct Matrix4x4
{
	float m[4][4];

	/// @brief 逆行列
	/// @return 
	Matrix4x4 Inverse() const;

	/// @brief 転置行列
	/// @return 
	Matrix4x4 Transpose() const;

	// 加算
	Matrix4x4 operator+=(const Matrix4x4& matrix)
	{
		for (int32_t i = 0; i < 4; ++i)for (int32_t j = 0; j < 4; ++j)
			this->m[i][j] += matrix.m[i][j];

		return *this;
	}

	// 減算
	Matrix4x4 operator-=(const Matrix4x4& matrix)
	{
		for (int32_t i = 0; i < 4; ++i)for (int32_t j = 0; j < 4; ++j)
			this->m[i][j] -= matrix.m[i][j];

		return *this;
	}

	// スカラー倍
	Matrix4x4 operator*=(float scalar)
	{
		for (int32_t i = 0; i < 4; ++i)for (int32_t j = 0; j < 4; ++j)
			this->m[i][j] *= scalar;

		return *this;
	}

	// 乗算
	Matrix4x4 operator*=(const Matrix4x4& matrix)
	{
		// 積
		Matrix4x4 multiply;

		multiply.m[0][0] = this->m[0][0] * matrix.m[0][0] + this->m[0][1] * matrix.m[1][0] + this->m[0][2] * matrix.m[2][0] + this->m[0][3] * matrix.m[3][0];
		multiply.m[0][1] = this->m[0][0] * matrix.m[0][1] + this->m[0][1] * matrix.m[1][1] + this->m[0][2] * matrix.m[2][1] + this->m[0][3] * matrix.m[3][1];
		multiply.m[0][2] = this->m[0][0] * matrix.m[0][2] + this->m[0][1] * matrix.m[1][2] + this->m[0][2] * matrix.m[2][2] + this->m[0][3] * matrix.m[3][2];
		multiply.m[0][3] = this->m[0][0] * matrix.m[0][3] + this->m[0][1] * matrix.m[1][3] + this->m[0][2] * matrix.m[2][3] + this->m[0][3] * matrix.m[3][3];

		multiply.m[1][0] = this->m[1][0] * matrix.m[0][0] + this->m[1][1] * matrix.m[1][0] + this->m[1][2] * matrix.m[2][0] + this->m[1][3] * matrix.m[3][0];
		multiply.m[1][1] = this->m[1][0] * matrix.m[0][1] + this->m[1][1] * matrix.m[1][1] + this->m[1][2] * matrix.m[2][1] + this->m[1][3] * matrix.m[3][1];
		multiply.m[1][2] = this->m[1][0] * matrix.m[0][2] + this->m[1][1] * matrix.m[1][2] + this->m[1][2] * matrix.m[2][2] + this->m[1][3] * matrix.m[3][2];
		multiply.m[1][3] = this->m[1][0] * matrix.m[0][3] + this->m[1][1] * matrix.m[1][3] + this->m[1][2] * matrix.m[2][3] + this->m[1][3] * matrix.m[3][3];

		multiply.m[2][0] = this->m[2][0] * matrix.m[0][0] + this->m[2][1] * matrix.m[1][0] + this->m[2][2] * matrix.m[2][0] + this->m[2][3] * matrix.m[3][0];
		multiply.m[2][1] = this->m[2][0] * matrix.m[0][1] + this->m[2][1] * matrix.m[1][1] + this->m[2][2] * matrix.m[2][1] + this->m[2][3] * matrix.m[3][1];
		multiply.m[2][2] = this->m[2][0] * matrix.m[0][2] + this->m[2][1] * matrix.m[1][2] + this->m[2][2] * matrix.m[2][2] + this->m[2][3] * matrix.m[3][2];
		multiply.m[2][3] = this->m[2][0] * matrix.m[0][3] + this->m[2][1] * matrix.m[1][3] + this->m[2][2] * matrix.m[2][3] + this->m[2][3] * matrix.m[3][3];

		multiply.m[3][0] = this->m[3][0] * matrix.m[0][0] + this->m[3][1] * matrix.m[1][0] + this->m[3][2] * matrix.m[2][0] + this->m[3][3] * matrix.m[3][0];
		multiply.m[3][1] = this->m[3][0] * matrix.m[0][1] + this->m[3][1] * matrix.m[1][1] + this->m[3][2] * matrix.m[2][1] + this->m[3][3] * matrix.m[3][1];
		multiply.m[3][2] = this->m[3][0] * matrix.m[0][2] + this->m[3][1] * matrix.m[1][2] + this->m[3][2] * matrix.m[2][2] + this->m[3][3] * matrix.m[3][2];
		multiply.m[3][3] = this->m[3][0] * matrix.m[0][3] + this->m[3][1] * matrix.m[1][3] + this->m[3][2] * matrix.m[2][3] + this->m[3][3] * matrix.m[3][3];

		*this = multiply;

		return *this;
	}
};

namespace
{
	/// @brief 加算
	/// @param m1 
	/// @param m2 
	/// @return 
	Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2)
	{
		// 加算
		Matrix4x4 add;

		for (int32_t i = 0; i < 4; ++i)for (int32_t j = 0; j < 4; ++j)
			add.m[i][j] = m1.m[i][j] + m2.m[i][j];

		return add;
	}

	/// @brief 減算
	/// @param m1 
	/// @param m2 
	/// @return 
	Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2)
	{
		// 加算
		Matrix4x4 subtract;

		for (int32_t i = 0; i < 4; ++i)for (int32_t j = 0; j < 4; ++j)
			subtract.m[i][j] = m1.m[i][j] - m2.m[i][j];

		return subtract;
	}

	/// @brief スカラー倍
	/// @param scalar 
	/// @param matrix 
	/// @return 
	Matrix4x4 operator*(float scalar, const Matrix4x4& matrix)
	{
		// スカラー倍
		Matrix4x4 scalarMultiply;

		for (int32_t i = 0; i < 4; ++i)for (int32_t j = 0; j < 4; ++j)
			scalarMultiply.m[i][j] = scalar * matrix.m[i][j];

		return scalarMultiply;
	}

	/// @brief スカラー倍
	/// @param matrix 
	/// @param scalar 
	/// @return 
	Matrix4x4 operator*(const Matrix4x4& matrix, float scalar)
	{
		// スカラー倍
		Matrix4x4 scalarMultiply;

		for (int32_t i = 0; i < 4; ++i)for (int32_t j = 0; j < 4; ++j)
			scalarMultiply.m[i][j] = matrix.m[i][j] * scalar;

		return scalarMultiply;
	}

	/// @brief 積
	/// @param m1 
	/// @param m2 
	/// @return 
	Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2)
	{
		// 積
		Matrix4x4 multiply;

		multiply.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0];
		multiply.m[0][1] = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1];
		multiply.m[0][2] = m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2];
		multiply.m[0][3] = m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] + m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3];

		multiply.m[1][0] = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0];
		multiply.m[1][1] = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1];
		multiply.m[1][2] = m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2];
		multiply.m[1][3] = m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] + m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3];

		multiply.m[2][0] = m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0];
		multiply.m[2][1] = m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1];
		multiply.m[2][2] = m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2];
		multiply.m[2][3] = m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] + m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3];

		multiply.m[3][0] = m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] + m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0];
		multiply.m[3][1] = m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] + m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1];
		multiply.m[3][2] = m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] + m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2];
		multiply.m[3][3] = m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] + m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3];

		return multiply;
	}
}


/// @brief 座標変換
/// @param vector ベクトル
/// @param matrix 行列
/// @return 
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

/// @brief 座標変換
/// @param vector ベクトル
/// @param matrix 行列
/// @return 
Vector4 Transform(const Vector4& vector, const Matrix4x4& matrix);

/// @brief 法線方向の座標変換
/// @param vector ベクトル
/// @param matrix 行列
/// @return 
Vector3 TransformNormal(const Vector3& vector, const Matrix4x4& matrix);


/// @brief 単位行列
/// @return 
Matrix4x4 MakeIdentityMatrix4x4();

/// @brief 拡大縮小行列
/// @param scale 大きさ
/// @return 
Matrix4x4 Make2DScaleMatrix4x4(const Vector2& scale);

/// @brief 拡大縮小行列
/// @param scale 大きさ
/// @return 
Matrix4x4 Make3DScaleMatrix4x4(const Vector3& scale);

/// @brief X軸回転行列
/// @param radian 角度
/// @return 
Matrix4x4 Make3DRotateXMatrix4x4(float radian);

/// @brief Y軸回転行列
/// @param radian 角度
/// @return 
Matrix4x4 Make3DRotateYMatrix4x4(float radian);

/// @brief Z軸回転行列
/// @param radian 角度
/// @return 
Matrix4x4 Make3DRotateZMatrix4x4(float radian);

/// @brief 回転行列
/// @param rotation オイラー角
/// @return 
Matrix4x4 Make3DRotateMatrix4x4(const Vector3& rotation);

/// @brief 回転行列
/// @param quaternion クォータニオン
/// @return 
Matrix4x4 Make3DRotateMatrix4x4(const Quaternion& quaternion);

/// @brief 平行移動行列
/// @param translate 平行移動
/// @return 
Matrix4x4 Make2DTranslateMatrix4x4(const Vector2& translate);

/// @brief 平行移動行列
/// @param translate 平行移動
/// @return 
Matrix4x4 Make3DTranslateMatrix4x4(const Vector3& translate);

/// @brief アフィン変換行列
/// @param scale 拡縮
/// @param rotation オイラー角
/// @param translate 平行移動
/// @return 
Matrix4x4 Make3DAffineMatrix4x4(const Vector2& scale, const Vector3& rotation, const Vector2& translate);

/// @brief アフィン変換行列
/// @param scale 拡縮
/// @param quaternion クォータニオン
/// @param translate 平行移動
/// @return 
Matrix4x4 Make3DAffineMatrix4x4(const Vector3& scale, const Quaternion& quaternion, const Vector3& translate);

/// @brief 透視投影行列 3D
/// @param fovY 画角
/// @param aspectRatio アスペクト比
/// @param nearClip 近平面の距離
/// @param farClip 遠平面の距離
/// @return 
Matrix4x4 MakePerspectiveFovMatrix4x4(float fovY, float aspectRatio, float nearClip, float farClip);

/// @brief 平行投影行列 2D
/// @param left 左
/// @param top 上
/// @param right 右
/// @param bottom 下
/// @param nearClip 近平面の距離
/// @param farClip 遠平面の距離
/// @return 
Matrix4x4 MakeOrthographicMatrix4x4(float left, float top, float right, float bottom, float nearClip, float farClip);

/// @brief ビューポート変換行列
/// @param left 左
/// @param top 上
/// @param width スクリーン横幅
/// @param height スクリーン縦幅
/// @param minDepth 最小深度値
/// @param maxDepth 最大深度値
/// @return 
Matrix4x4 MakeViewportMatrix4x4(float left, float top, float width, float height, float minDepth, float maxDepth);