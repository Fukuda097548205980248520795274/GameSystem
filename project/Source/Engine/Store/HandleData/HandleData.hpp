#pragma once
#include <cstdint>

namespace Detail
{
	/// @brief 基底ハンドル
	template <typename Tag>
	struct Handle
	{
		// インデックス
		uint32_t index = 0;

		// 世代番号
		uint32_t generation = 0;

		/// @brief 有効なハンドルかどうかを判定する
		/// @return
		bool IsValid() const { return generation != 0; }

		// 比較演算子
		bool operator==(const Handle& other) const { return index == other.index && generation == other.generation; }
		bool operator!=(const Handle& other) const { return !(*this == other); }

		/// @brief 無効なハンドルを取得する
		/// @return 
		static Handle Invalid() { return Handle{ 0, 0 }; }
	};


}