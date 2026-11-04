#pragma once
#include "Store/AnimationStore/AnimationData.hpp"

namespace Detail
{
	/// @brief アニメーションの読み込み
	/// @param directoryPath 
	/// @param filename 
	/// @return 
	AnimationData LoadAnimation(const std::string& filePath);

	/// @brief 任意の時刻の値を取得する
	/// @param keyframe 
	/// @param time 
	/// @return 
	Vector3 CalculateValue(const std::vector<KeyFrameVector3>& keyframe, float time);

	/// @brief 任意の時刻の値を取得する
	/// @param keyframe 
	/// @param time 
	/// @return 
	Quaternion CalculateValue(const std::vector<KeyFrameQuaternion>& keyframe, float time);
}