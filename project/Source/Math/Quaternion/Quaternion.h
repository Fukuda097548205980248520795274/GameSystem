#pragma once
#include "../Vector/Vector3/Vector3.h"

struct Quaternion
{
	float x;
	float y;
	float z;
	float w;


	/// @brief ノルム
	/// @return 
	float Norm() const;

	/// @brief 正規化
	/// @return 
	Quaternion Normalize() const;

	/// @brief 共役
	/// @return 
	Quaternion Conjugate() const;

	/// @brief 逆
	/// @return 
	Quaternion Inverse() const;
};

namespace
{
	/// @brief 加算
	/// @param q1 
	/// @param q2 
	/// @return 
	Quaternion operator+(const Quaternion& q1, const Quaternion& q2)
	{
		// 加算
		Quaternion add;
		add.x = q1.x + q2.x;
		add.y = q1.y + q2.y;
		add.z = q1.z + q2.z;
		add.w = q1.w + q2.w;

		return add;
	}

	/// @brief 減算
	/// @param q1 
	/// @param q2 
	/// @return 
	Quaternion operator-(const Quaternion& q1, const Quaternion& q2)
	{
		// 減算
		Quaternion subtract;
		subtract.x = q1.x - q2.x;
		subtract.y = q1.y - q2.y;
		subtract.z = q1.z - q2.z;
		subtract.w = q1.w - q2.w;

		return subtract;
	}

	/// @brief スカラー倍
	/// @param quaternion クォータニオン
	/// @param scalar スカラー
	/// @return 
	Quaternion operator*(const Quaternion& quaternion, float scalar)
	{
		// スカラー倍
		Quaternion multiply;
		multiply.x = quaternion.x * scalar;
		multiply.y = quaternion.y * scalar;
		multiply.z = quaternion.z * scalar;
		multiply.w = quaternion.w * scalar;
		return multiply;
	}

	/// @brief スカラー倍
	/// @param scalar スカラー
	/// @param quaternion クォータニオン
	/// @return 
	Quaternion operator*(float scalar, const Quaternion& quaternion)
	{
		// スカラー倍
		Quaternion multiply;
		multiply.x = scalar * quaternion.x;
		multiply.y = scalar * quaternion.y;
		multiply.z = scalar * quaternion.z;
		multiply.w = scalar * quaternion.w;
		return multiply;
	}

	/// @brief スカラー除算
	/// @param quaternion クォータニオン
	/// @param scalar スカラー
	/// @return 
	Quaternion operator/(const Quaternion& quaternion, float scalar)
	{
		// スカラー除算
		Quaternion division;
		division.x = quaternion.x / scalar;
		division.y = quaternion.y / scalar;
		division.z = quaternion.z / scalar;
		division.w = quaternion.w / scalar;
		return division;
	}

	/// @brief スカラー除算
	/// @param scalar スカラー
	/// @param quaternion クォータニオン
	/// @return 
	Quaternion operator/(float scalar, const Quaternion& quaternion)
	{
		// スカラー除算
		Quaternion division;
		division.x = scalar / quaternion.x;
		division.y = scalar / quaternion.y;
		division.z = scalar / quaternion.z;
		division.w = scalar / quaternion.w;
		return division;
	}


	/// @brief 積
	/// @param q1 
	/// @param q2 
	/// @return 
	Quaternion operator*(const Quaternion& q1, const Quaternion& q2)
	{
		// ベクトルに変換する
		Vector3 v1 = Vector3(q1.x, q1.y, q1.z);
		Vector3 v2 = Vector3(q2.x, q2.y, q2.z);

		// ベクトル部の積
		Vector3 vecMultiply = Cross(v1, v2) + q2.w * v1 + q1.w * v2;

		// 積
		Quaternion multiply;
		multiply.x = vecMultiply.x;
		multiply.y = vecMultiply.y;
		multiply.z = vecMultiply.z;
		multiply.w = q1.w * q2.w - Dot(v1, v2);

		return multiply;
	}

	// +
	Quaternion operator+(const Quaternion& quaternion)
	{
		return quaternion;
	}

	// -
	Quaternion operator-(const Quaternion& quaternion)
	{
		Quaternion minus = Quaternion(-quaternion.x, -quaternion.y, -quaternion.z, -quaternion.w);
		return minus;
	}
}

/// @brief 乗法単位元
/// @return 
Quaternion MakeIdentityQuaternion();

/// @brief 角度をクォータニオンに変換する
/// @param angle 角度
/// @param axis 軸
/// @return 
Quaternion ToQuaternion(float angle, const Vector3& axis);

/// @brief 内積
/// @param q1 
/// @param q2 
/// @return 
float Dot(const Quaternion& q1, const Quaternion& q2);

/// @brief 球面線形補間
/// @param q1 
/// @param q2 
/// @param t 
/// @return 
Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t);