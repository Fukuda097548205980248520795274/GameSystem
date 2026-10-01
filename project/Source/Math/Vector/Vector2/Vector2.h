#pragma once
#include <cmath>

/// @brief 2次元ベクトル
template<typename T>
struct TVector2
{
	T x;
	T y;

	// コンストラクタ
	TVector2() : x(T(0)), y(T(0)) {}
	TVector2(T x, T y) : x(x), y(y) {}

	/// @brief 長さ
	/// @return 
	float Length() const
	{
		return std::sqrt(x * x + y * y);
	}

	/// @brief 長さの二乗
	/// @return 
	float LengthSq() const
	{
		return x * x + y * y;
	}

	/// @brief 正規化
	/// @return 
	TVector2<float> Normalize() const
	{
		// 長さを取得
		float length = Length();

		// 長さが0の場合は正規化できないので、0ベクトルを返す
		if (length < 1e-5f)
			return TVector2<float>(0.0f, 0.0f);

		// 正規化
		return TVector2<float>(x / length, y / length);
	}


	// 加算
	TVector2 operator+=(const TVector2& vector)
	{
		this->x += vector.x;
		this->y += vector.y;
		return *this;
	}

	// 減算
	TVector2 operator-=(const TVector2& vector)
	{
		this->x -= vector.x;
		this->y -= vector.y;
		return *this;
	}

	// スカラー倍
	TVector2 operator*=(T scalar)
	{
		this->x *= scalar;
		this->y *= scalar;
		return *this;
	}

	// スカラー除算
	TVector2 operator/=(T scalar)
	{
		this->x /= scalar;
		this->y /= scalar;
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
	TVector2<T> operator+(const TVector2<T>& v1, const TVector2<T>& v2)
	{
		TVector2<T> add = TVector2<T>(T(0), T(0));
		add.x = v1.x + v2.x;
		add.y = v1.y + v2.y;
		return add;
	}

	/// @brief 減算
	/// @param v1 
	/// @param v2 
	/// @return 
	template<typename T>
	TVector2<T> operator-(const TVector2<T>& v1, const TVector2<T>& v2)
	{
		TVector2<T> subtract = TVector2<T>(T(0), T(0));
		subtract.x = v1.x - v2.x;
		subtract.y = v1.y - v2.y;
		return subtract;
	}

	/// @brief スカラー倍
	/// @param scalar 
	/// @param vector 
	/// @return 
	template<typename T>
	TVector2<T> operator*(T scalar, const TVector2<T>& vector)
	{
		TVector2<T> multiply = TVector2<T>(T(0), T(0));
		multiply.x = scalar * vector.x;
		multiply.y = scalar * vector.y;
		return multiply;
	}

	/// @brief スカラー倍
	/// @param vector 
	/// @param scalar 
	/// @return 
	template<typename T>
	TVector2<T> operator*(const TVector2<T>& vector, T scalar)
	{
		TVector2<T> multiply = TVector2<T>(T(0), T(0));
		multiply.x = vector.x * scalar;
		multiply.y = vector.y * scalar;
		return multiply;
	}

	/// @brief スカラー除算
	/// @param scalar 
	/// @param vector 
	/// @return 
	template<typename T>
	TVector2<T> operator/(T scalar, const TVector2<T>& vector)
	{
		TVector2<T> division = TVector2<T>(T(0), T(0));
		division.x = scalar / vector.x;
		division.y = scalar / vector.y;
		return division;
	}

	/// @brief スカラー除算
	/// @param vector 
	/// @param scalar 
	/// @return 
	template<typename T>
	TVector2<T> operator/(const TVector2<T>& vector, T scalar)
	{
		TVector2<T> division = TVector2<T>(T(0), T(0));
		division.x = vector.x / scalar;
		division.y = vector.y / scalar;
		return division;
	}

	/// @brief +
	/// @param vector 
	/// @return 
	template<typename T>
	TVector2<T> operator+(const TVector2<T>& vector)
	{
		return vector;
	}

	/// @brief -
	/// @param vector 
	/// @return 
	template<typename T>
	TVector2<T> operator-(const TVector2<T>& vector)
	{
		return TVector2<T>(-vector.x, -vector.y);
	}
}

using Vector2 = TVector2<float>;
using Vector2int = TVector2<int>;

/// @brief 内積
/// @param v1 
/// @param v2 
/// @return 
float Dot(const Vector2& v1, const Vector2& v2);

/// @brief クロス積
/// @param v1 
/// @param v2 
/// @return 
float Cross(const Vector2& v1, const Vector2& v2);

/// @brief 射影ベクトル
/// @param line 線
/// @param point 点
/// @return 
Vector2 Project(const Vector2& line, const Vector2& point);

/// @brief 反射ベクトル
/// @param input 入射ベクトル
/// @param normal 法線
/// @return 
Vector2 Reflect(const Vector2& input, const Vector2& normal);

/// @brief 線形補間
/// @param v1 
/// @param v2 
/// @param t 
/// @return 
Vector2 Lerp(const Vector2& v1, const Vector2& v2, float t);