#pragma once
#include "Vector/Vector2/Vector2.hpp"
#include "Vector/Vector3/Vector3.hpp"
#include "Vector/Vector4/Vector4.hpp"
#include <concepts>


template<typename T>
concept literal = std::is_arithmetic_v<T> || std::is_same_v<T, Vector2> || std::is_same_v<T, Vector3> || std::is_same_v<T, Vector4>;

/// @brief 線形補間
/// @tparam T 
/// @param a 
/// @param b 
/// @param t 
/// @return 
template<literal T>
T Lerp(const T& a, const T& b, float t)
{
	return a + (b - a) * t;
}