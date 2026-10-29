#pragma once
#include <random>

/// @brief 範囲で乱数を取得する
/// @tparam T 
/// @param min 
/// @param max 
/// @return 
template<typename T>
T GetRandomRange(T min, T max)
{
	if (max < min)
	{
		max = min;
	}

	static std::random_device generator;
	static std::mt19937 randomEngine(generator());

	// int型
	if constexpr (std::is_integral_v<T>)
	{
		std::uniform_int_distribution<T> dist(min, max);
		return dist(randomEngine);
	}
	else if constexpr (std::is_floating_point_v<T>)
	{
		// float double 型
		std::uniform_real_distribution<T> dist(min, max);
		return dist(randomEngine);
	}
	else
	{
		// それ以外はエラー
		static_assert(false, "Unsupported type for random range");
	}
}