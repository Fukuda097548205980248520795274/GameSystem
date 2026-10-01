#pragma once
#include <cmath>

/// @brief 3次元ベクトル
template<typename T>
struct TVector3
{
	T x;
	T y;
	T z;

	// コンストラクタ
	TVector3() : x(T(0)), y(T(0)), z(T(0)) {}
	TVector3(T x, T y, T z) : x(x), y(y), z(z) {}

	/// @brief 長さ
	/// @return 
	float Length() const
	{
		return std::sqrt(x * x + y * y + z * z);
	}

	/// @brief 長さの二乗
	/// @return 
	float LengthSq()const
	{
		return x * x + y * y + z * z;
	}

	/// @brief 正規化
	/// @return 
	TVector3<float> Normalize() const
	{
		// 長さを取得
		float length = Length();

		// 長さが0の場合は正規化できないので、0ベクトルを返す
		if (length < 1e-5f)
			return TVector3<float>(0.0f, 0.0f, 0.0f);

		// 正規化
		return TVector3<float>(x / length, y / length, z / length);
	}


	/// @brief 加算する
	/// @param vector 
	/// @return 
	TVector3 operator+=(const TVector3& vector)
	{
		this->x += vector.x;
		this->y += vector.y;
		this->z += vector.z;
		return *this;
	}

	/// @brief 減算する
	/// @param vector 
	/// @return 
	TVector3 operator-=(const TVector3& vector)
	{
		this->x -= vector.x;
		this->y -= vector.y;
		this->z -= vector.z;
		return *this;
	}

	/// @brief スカラー倍
	/// @param scalar 
	/// @return 
	TVector3 operator*=(T scalar)
	{
		this->x *= scalar;
		this->y *= scalar;
		this->z *= scalar;
		return *this;
	}

	/// @brief スカラー除算
	/// @param scalar 
	/// @return 
	TVector3 operator/=(T scalar)
	{
		this->x /= scalar;
		this->y /= scalar;
		this->z /= scalar;
		return *this;
	}
};

namespace
{
	/// @brief 加算
	/// @param v1 
	/// @param v2 
	/// @return 
	template<typename T>
	TVector3<T> operator+(const TVector3<T>& v1, const TVector3<T>& v2)
	{
		TVector3<T> add = TVector3(T(0), T(0), T(0));
		add.x = v1.x + v2.x;
		add.y = v1.y + v2.y;
		add.z = v1.z + v2.z;
		return add;
	}

	/// @brief 減算
	/// @param v1 
	/// @param v2 
	/// @return 
	template<typename T>
	TVector3<T> operator-(const TVector3<T>& v1, const TVector3<T>& v2)
	{
		TVector3<T> subtract = TVector3<T>(T(0), T(0), T(0));
		subtract.x = v1.x - v2.x;
		subtract.y = v1.y - v2.y;
		subtract.z = v1.z - v2.z;
		return subtract;
	}

	/// @brief スカラー倍
	/// @param scalar 
	/// @param vector 
	/// @return 
	template<typename T>
	TVector3<T> operator*(T scalar, const TVector3<T>& vector)
	{
		TVector3<T> multiply = TVector3<T>(T(0), T(0), T(0));
		multiply.x = scalar * vector.x;
		multiply.y = scalar * vector.y;
		multiply.z = scalar * vector.z;
		return multiply;
	}

	/// @brief スカラー倍
	/// @param vector 
	/// @param scalar 
	/// @return 
	template<typename T>
	TVector3<T> operator*(const TVector3<T>& vector, T scalar)
	{
		TVector3<T> multiply = TVector3<T>(T(0), T(0), T(0));
		multiply.x = vector.x * scalar;
		multiply.y = vector.y * scalar;
		multiply.z = vector.z * scalar;
		return multiply;
	}

	/// @brief スカラー除算
	/// @param vector 
	/// @param scalar 
	/// @return 
	template<typename T>
	TVector3<T> operator/(const TVector3<T>& vector, T scalar)
	{
		TVector3<T> division = TVector3<T>(T(0), T(0), T(0));
		division.x = vector.x / scalar;
		division.y = vector.y / scalar;
		division.z = vector.z / scalar;
		return division;
	}

	/// @brief +
	/// @param vector 
	/// @return 
	template<typename T>
	TVector3<T> operator+(const TVector3<T>& vector)
	{
		return vector;
	}

	/// @brief -
	/// @param vector 
	/// @return 
	template<typename T>
	TVector3<T> operator-(const TVector3<T>& vector)
	{
		return TVector3<T>(-vector.x, -vector.y, -vector.z);
	}
}

using Vector3 = TVector3<float>;
using Vector3int = TVector3<int>;

/// @brief 内積
/// @param v1 
/// @param v2 
/// @return 
float Dot(const Vector3& v1, const Vector3& v2);

/// @brief クロス積
/// @param v1 
/// @param v2 
/// @return 
Vector3 Cross(const Vector3& v1, const Vector3& v2);

/// @brief 射影ベクトル
/// @param line 線
/// @param point 点
/// @return 
Vector3 Project(const Vector3& line, const Vector3& point);

/// @brief 反射ベクトル
/// @param input 入射ベクトル
/// @param normal 法線
/// @return 
Vector3 Reflect(const Vector3& input, const Vector3& normal);

/// @brief 球面座標系
/// @param radius 半径
/// @param theta 
/// @param phi 
/// @return 
Vector3 SphericalCoordinate(float radius, float theta, float phi);

/// @brief 線形補間
/// @param start 
/// @param end 
/// @param t
/// @return 
Vector3 Lerp(const Vector3& start, const Vector3& end, float t);