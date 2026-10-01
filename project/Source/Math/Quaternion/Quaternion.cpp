#include "Quaternion.h"
#include <cmath>

/// @brief ノルム
/// @return 
float Quaternion::Norm() const
{
	return std::sqrtf(w * w + x * x + y * y + z * z);
}

/// @brief 正規化
/// @return 
Quaternion Quaternion::Normalize() const
{
	// ノルム
	float norm = this->Norm();

	// 正規化した値
	Quaternion normalize = Quaternion(0.0f, 0.0f, 0.0f, 0.0f);

	if (norm != 0.0f)
	{
		normalize = *this / norm;
	}

	return normalize;
}

/// @brief 共役
/// @return 
Quaternion Quaternion::Conjugate() const
{
	return Quaternion(-x, -y, -z, w);
}

/// @brief 逆
/// @return 
Quaternion Quaternion::Inverse() const
{
	return this->Conjugate() / std::powf(this->Norm(), 2.0f);
}

/// @brief 乗法単位元
/// @return 
Quaternion MakeIdentityQuaternion()
{
	return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
}

/// @brief 内積
/// @param q1 
/// @param q2 
/// @return 
float Dot(const Quaternion& q1, const Quaternion& q2)
{
	return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

/// @brief 角度をクォータニオンに変換する
/// @param angle 角度
/// @param axis 軸
/// @return 
Quaternion ToQuaternion(float angle, const Vector3& axis)
{
	Vector3 n = axis.Normalize();

	Quaternion q;
	q.w = std::cos(angle / 2.0f);
	q.x = n.x * std::sin(angle / 2.0f);
	q.y = n.y * std::sin(angle / 2.0f);
	q.z = n.z * std::sin(angle / 2.0f);
	return q;
}

/// @brief 球面線形補間
/// @param q1 
/// @param q2 
/// @param t 
/// @return 
Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t)
{
	// q1 のコピー
	Quaternion q0Copy = q1;

	// 内積
	float dot = Dot(q0Copy, q2);

	// 最短経路を確保
	if (dot < 0.0f)
	{
		q0Copy = -q0Copy;
		dot = -dot;
	}

	const float kDotThreshold = 1.0f - 1e-4f;

	// 非常に近い場合は Lerp
	if (dot > kDotThreshold)
	{
		Quaternion result = (1.0f - t) * q0Copy + t * q2;
		return result.Normalize();
	}

	// 通常の Slerp
	float theta = std::acos(dot);
	float sinTheta = std::sin(theta);

	float scale0 = std::sin((1.0f - t) * theta) / sinTheta;
	float scale1 = std::sin(t * theta) / sinTheta;

	return scale0 * q0Copy + scale1 * q2;
}