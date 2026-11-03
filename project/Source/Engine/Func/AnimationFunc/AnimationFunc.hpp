#pragma once
#include "Store/AnimationStore/AnimationData.hpp"

namespace Detail
{
	/// @brief アニメーションの読み込み
	/// @param directoryPath 
	/// @param filename 
	/// @return 
	AnimationData LoadAnimation(const std::string& filePath);
}