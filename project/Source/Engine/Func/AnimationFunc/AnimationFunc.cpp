#include "AnimationFunc.hpp"
#include "Func/MathFunc.hpp"
#include <cassert>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

/// @brief アニメーションの読み込み
/// @param directoryPath 
/// @param filename 
/// @return 
Detail::AnimationData Detail::LoadAnimation(const std::string& filePath)
{
	AnimationData animation;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);

	// アニメーションがないといけいない
	if (scene->mNumAnimations == 0)
	{
		return animation;
	}

	// 最初のアニメーションだけ採用
	aiAnimation* animationAssImp = scene->mAnimations[0];

	// 時間の単位を秒に変換
	animation.duration = float(animationAssImp->mDuration / animationAssImp->mTicksPerSecond);

	// 領域確保
	animation.nodes.resize(animationAssImp->mNumChannels);

	for (uint32_t channelIndex = 0; channelIndex < animationAssImp->mNumChannels; ++channelIndex)
	{
		aiNodeAnim* nodeAnimationAssImp = animationAssImp->mChannels[channelIndex];

		// インデックスを記録する
		std::string animationName = nodeAnimationAssImp->mNodeName.C_Str();
		animation.indices[animationName] = channelIndex;

		// ノードを取得する
		NodeAnimation& node = animation.nodes[channelIndex];


		// 位置
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssImp->mNumPositionKeys; ++keyIndex)
		{
			aiVectorKey& keyAssImp = nodeAnimationAssImp->mPositionKeys[keyIndex];

			KeyFrameVector3 keyframe;

			// 秒に変換
			keyframe.time = float(keyAssImp.mTime / animationAssImp->mTicksPerSecond);

			// 右手 -> 左手
			keyframe.value = { -keyAssImp.mValue.x, keyAssImp.mValue.y,keyAssImp.mValue.z };
			node.translate.push_back(keyframe);
		}

		// 回転
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssImp->mNumRotationKeys; ++keyIndex)
		{
			aiQuatKey& keyAssImp = nodeAnimationAssImp->mRotationKeys[keyIndex];

			KeyFrameQuaternion keyframe;

			// 秒に変換
			keyframe.time = float(keyAssImp.mTime / animationAssImp->mTicksPerSecond);

			// 右手 -> 左手
			keyframe.value = { keyAssImp.mValue.x, -keyAssImp.mValue.y, -keyAssImp.mValue.z , keyAssImp.mValue.w };
			node.rotate.push_back(keyframe);
		}

		// 拡縮
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssImp->mNumScalingKeys; ++keyIndex)
		{
			aiVectorKey& keyAssImp = nodeAnimationAssImp->mScalingKeys[keyIndex];

			KeyFrameVector3 keyframe;

			// 秒に変換
			keyframe.time = float(keyAssImp.mTime / animationAssImp->mTicksPerSecond);

			// そのまま
			keyframe.value = { keyAssImp.mValue.x, keyAssImp.mValue.y,keyAssImp.mValue.z };
			node.scale.push_back(keyframe);
		}
	}

	// ファイルパスを記録する
	animation.filePath = filePath;

	return animation;
}

/// @brief 任意の時刻の値を取得する
/// @param keyframe 
/// @param time 
/// @return 
Vector3 Detail::CalculateValue(const std::vector<KeyFrameVector3>& keyframe, float time)
{
	// キーがないと返す値が内のでアウト
	assert(!keyframe.empty());

	// キーが一つか、時刻がキーフレーム前なら最初の値とする
	if (keyframe.size() == 1 || time <= keyframe[0].time)
	{
		return keyframe[0].value;
	}

	for (size_t index = 0; index < keyframe.size() - 1; ++index)
	{
		size_t nextIndex = index + 1;

		// index と nextIndex
		if (keyframe[index].time <= time && time <= keyframe[nextIndex].time)
		{
			// 範囲内を補完する
			float t = (time - keyframe[index].time) / (keyframe[nextIndex].time - keyframe[index].time);
			return Lerp<Vector3>(keyframe[index].value, keyframe[nextIndex].value, t);
		}
	}

	// ここまで来たら、時刻より後ろなので、最後の値を返す
	return (*keyframe.rbegin()).value;
}

/// @brief 任意の時刻の値を取得する
/// @param keyframe 
/// @param time 
/// @return 
Quaternion Detail::CalculateValue(const std::vector<KeyFrameQuaternion>& keyframe, float time)
{
	// キーがないと返す値が内のでアウト
	assert(!keyframe.empty());

	// キーが一つか、時刻がキーフレーム前なら最初の値とする
	if (keyframe.size() == 1 || time <= keyframe[0].time)
	{
		return keyframe[0].value;
	}

	for (size_t index = 0; index < keyframe.size() - 1; ++index)
	{
		size_t nextIndex = index + 1;

		// index と nextIndex
		if (keyframe[index].time <= time && time <= keyframe[nextIndex].time)
		{
			// 範囲内を補完する
			float t = (time - keyframe[index].time) / (keyframe[nextIndex].time - keyframe[index].time);
			return Slerp(keyframe[index].value, keyframe[nextIndex].value, t);
		}

		int a = 0;
	}

	// ここまで来たら、時刻より後ろなので、最後の値を返す
	return (*keyframe.rbegin()).value;
}