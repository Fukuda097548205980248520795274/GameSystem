#pragma once
#include "Store/ModelStore/ModelData.hpp"
#include "Store/SkeletonStore/SkeletonData.hpp"
#include "Store/AnimationStore/AnimationData.hpp"

namespace Detail
{
	/// @brief ボーンを読み込む
	/// @param filePath 
	/// @param skeleton 
	/// @return 
	ModelBoneData LoadBone(const std::string& filePath, const Skeleton& skeleton);

	/// @brief スケルトンを作成する
	/// @param rootNode 
	/// @return 
	Skeleton CreateSkeleton(const std::vector<ModelNode>& nodes);

	/// @brief ジョイントを作成する
	/// @param node 
	/// @param parent 
	/// @param joints 
	/// @return 
	int32_t CreateJoint(const std::vector<ModelNode>& nodes, const ModelNode& node, const std::optional<int32_t>& parent, std::vector<JointNode>& joints);

	/// @brief ボーンアニメーションを再生する
	/// @param skeleton 
	/// @param animation 
	/// @param animationTime 
	Skeleton ApplyBoneAnimation(const Skeleton& skeleton, const AnimationData& animation, float animationTime);

	/// @brief スケルトンのフォワードキネマティクス
	/// @param skeleton 
	/// @param pose 
	void ForwardKinematics(Skeleton& skeleton, SkeletonPose& pose);

	/// @brief スケルトンのインバースキネマティクス
	/// @param skeleton 
	/// @param pose 
	/// @param boneName 
	/// @param targetPosition
	void InverseKinematics(Skeleton& skeleton, SkeletonPose& pose, const std::string& effectedBoneName, const Vector3& targetPosition);
}