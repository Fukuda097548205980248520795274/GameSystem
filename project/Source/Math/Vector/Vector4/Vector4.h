#pragma once

/// @brief 4次元ベクトル
template<typename T>
struct TVector4
{
	T x;
	T y;
	T z;
	T w;

	// コンストラクタ
	TVector4() : x(T(0)), y(T(0)), z(T(0)), w(T(0)) {}
	TVector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
};

using Vector4 = TVector4<float>;
using Vector4int = TVector4<int>;

/// @brief 線形補間を行う
/// @param a 
/// @param b 
/// @param t 
/// @return 
Vector4 Lerp(const Vector4& a, const Vector4& b, float t);