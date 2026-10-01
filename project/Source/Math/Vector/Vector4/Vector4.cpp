#include "Vector4.h"

/// @brief 線形補間を行う
/// @param a 
/// @param b 
/// @param t 
/// @return 
Vector4 Lerp(const Vector4& a, const Vector4& b, float t)
{
	float x = (1.0f - t) * a.x + t * b.x;
	float y = (1.0f - t) * a.y + t * b.y;
	float z = (1.0f - t) * a.z + t * b.z;
	float w = (1.0f - t) * a.w + t * b.w;

	return Vector4{ x, y, z, w };
}