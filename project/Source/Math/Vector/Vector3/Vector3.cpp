#include "Vector3.h"
#include <cmath>

/// @brief 内積
/// @param v1 
/// @param v2 
/// @return 
float Dot(const Vector3& v1, const Vector3& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

/// @brief クロス積
/// @param v1 
/// @param v2 
/// @return 
Vector3 Cross(const Vector3& v1, const Vector3& v2)
{
	Vector3 cross = Vector3(0.0f, 0.0f, 0.0f);
	cross.x = v1.y * v2.z - v1.z * v2.y;
	cross.y = v1.z * v2.x - v1.x * v2.z;
	cross.z = v1.x * v2.y - v1.y * v2.x;
	return cross;
}

/// @brief 射影ベクトル
/// @param line 線
/// @param point 点
/// @return 
Vector3 Project(const Vector3& line, const Vector3& point)
{
	Vector3 normal = line.Normalize();
	return Dot(normal, point) * normal;
}

/// @brief 反射ベクトル
/// @param input 入射ベクトル
/// @param normal 法線
/// @return 
Vector3 Reflect(const Vector3& input, const Vector3& normal)
{
	Vector3 normal2 = normal.Normalize();
	return input - (2.0f * (Dot(input, normal2) * normal2));
}

/// @brief 球面座標系
/// @param radius 半径
/// @param theta 
/// @param phi 
/// @return 
Vector3 SphericalCoordinate(float radius, float theta, float phi)
{
	// 座標
	TVector3<float> coordinate = TVector3<float>(0.0f, 0.0f, 0.0f);
	coordinate.x = radius * (std::cos(theta) * std::cos(phi));
	coordinate.y = radius * std::sin(theta);
	coordinate.z = radius * (std::cos(theta) * std::sin(phi));
	return coordinate;
}

/// @brief 線形補間
/// @param start 
/// @param end 
/// @param t 
/// @return 
Vector3 Lerp(const Vector3& start, const Vector3& end, float t)
{
	return (1.0f - t) * start + t * end;
}