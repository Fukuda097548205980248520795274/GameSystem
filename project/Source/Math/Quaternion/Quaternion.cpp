#include "Quaternion.hpp"
#include <cmath>
#include <numbers>

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

/// @brief オイラー角に変換する
/// @return 
Vector3 Quaternion::ToEulerAngles() const
{
	// オイラー角を計算する
	Vector3 eulerAngles;
	eulerAngles.x = std::atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
	eulerAngles.y = std::asin(2.0f * (w * y - z * x));
	eulerAngles.z = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));

	return eulerAngles;
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

/// @brief 方向ベクトルからクォータニオンを作成する
/// @param direction 
/// @return 
Quaternion LookRotation(const Vector3& direction)
{
    // 方向ベクトルを正規化する
    Vector3 f = direction.Normalize();

    // 内積を計算する
    float dot = Dot(f, Vector3(0.0f, 0.0f, 1.0f));

    // 内積が1に近い場合は、単位クォータニオンを返す
    if (dot >= 1.0f - 1e-6f)
    {
        return MakeIdentityQuaternion();
    }

    // 内積が-1に近い場合は、180度回転するクォータニオンを返す
    if (dot <= -1.0f + 1e-6f)
    {
        // 180度回転する軸を計算する
        Vector3 axis = Cross(Vector3(1.0f, 0.0f, 0.0f), f);

        // 軸がゼロベクトルの場合は、別の軸を使用する
        if (axis.LengthSq() < 1e-6f)
        {
            axis = Cross(Vector3(0.0f, 1.0f, 0.0f), f);
        }

        // 180度回転するクォータニオンを返す
        return ToQuaternion(std::numbers::pi_v<float>, axis.Normalize());
    }

    // 回転軸を計算する
    Vector3 axis = Cross(f, Vector3(0.0f, 0.0f, 1.0f));

    // クォータニオンを計算する
    Quaternion q;
    q.x = axis.x;
    q.y = axis.y;
    q.z = axis.z;
    q.w = 1.0f + dot;

    // 正規化して返す
    return q.Normalize();
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
