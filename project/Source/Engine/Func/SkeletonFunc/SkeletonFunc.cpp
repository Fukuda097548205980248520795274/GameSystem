#include "SkeletonFunc.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "Func/AnimationFunc/AnimationFunc.hpp"

/// @brief ボーンを読み込む
/// @param directory 
/// @param fileName 
/// @param skeleton 
/// @return 
Detail::ModelBoneData Detail::LoadBone(const std::string& filePath, const Skeleton& skeleton)
{
	// AssImpでファイルを開く
	Assimp::Importer importer;

	// シーンのデータ
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
		aiProcess_RemoveRedundantMaterials | aiProcess_CalcTangentSpace);

	// メッシュがないのは対応しない
	assert(scene->HasMeshes());

	// モデルボーンデータ
	ModelBoneData modelData;

	// メッシュ数が決まっていなかったら確保する
	if (modelData.meshes.empty())
	{
		modelData.meshes.resize(scene->mNumMeshes);
	}

	// メッシュ
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		// メッシュのデータ
		aiMesh* mesh = scene->mMeshes[meshIndex];

		// 法線がないメッシュは非対応
		assert(mesh->HasNormals());

		// UV座標がないのは非対応
		assert(mesh->HasTextureCoords(0));


		modelData.meshes[meshIndex].jointWeights.clear();

		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			aiBone* aiBone = mesh->mBones[boneIndex];
			std::string jointName = aiBone->mName.C_Str();
			JointWeightData& jointWeight = modelData.meshes[meshIndex].jointWeights[jointName];


			aiMatrix4x4 bindPoseMatrixAssimp = aiBone->mOffsetMatrix.Inverse();
			aiVector3D scaleAssimp, translateAssimp;
			aiQuaternion rotateAssimp;
			bindPoseMatrixAssimp.Decompose(scaleAssimp, rotateAssimp, translateAssimp);

			// トランスフォームに変える
			Vector3 scale = Vector3(scaleAssimp.x, scaleAssimp.y, scaleAssimp.z);
			Quaternion rotate = Quaternion(rotateAssimp.x, -rotateAssimp.y, -rotateAssimp.z, rotateAssimp.w);
			Vector3 translate = Vector3(-translateAssimp.x, translateAssimp.y, translateAssimp.z);

			// 行列にする
			jointWeight.inverseBindPoseMatrix = Make3DAffineMatrix4x4(scale, rotate, translate).Inverse();


			// 頂点ウェイトを追加
			for (uint32_t w = 0; w < aiBone->mNumWeights; ++w)
			{
				const aiVertexWeight& aiWeight = aiBone->mWeights[w];

				VertexWeightData vw;
				vw.vertexIndex = aiWeight.mVertexId;
				vw.weight = aiWeight.mWeight;

				jointWeight.vertexWeights.push_back(vw);
			}
		}

	}

	return modelData;
}

/// @brief スケルトンを作成する
/// @param rootNode 
/// @return 
Detail::Skeleton Detail::CreateSkeleton(const std::vector<ModelNode>& nodes)
{
	Skeleton skeleton;
	skeleton.rootIndex = Detail::CreateJoint(nodes, nodes[0], {}, skeleton.joints);

	// 名前とインデックスのマッピングを行いアクセスしやすくする
	for (const JointNode& joint : skeleton.joints) // Joint -> JointNode
	{
		skeleton.jointMap.emplace(joint.name, joint.index);
	}

	return skeleton;
}

/// @brief ジョイントを作成する
/// @param node 
/// @param parent 
/// @param joints 
/// @return 
int32_t Detail::CreateJoint(const std::vector<ModelNode>& nodes, const ModelNode& node, const std::optional<int32_t>& parent, std::vector<JointNode>& joints)
{
	JointNode joint;
	joint.name = node.name;
	joint.index = int32_t(joints.size());
	joint.parent = parent;

	// JointNodeの定義に合わせて初期トランスフォームのみを設定
	joint.initialTransform.scale = node.transform.scale;
	joint.initialTransform.rotate = node.transform.rotate;
	joint.initialTransform.translate = node.transform.translate;

	joints.push_back(joint);

	for (int32_t child : node.children)
	{
		int32_t childIndex = CreateJoint(nodes, nodes[child], joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}

	return joint.index;
}

/// @brief ボーンアニメーションを再生する
/// @param skeleton 
/// @param animation 
/// @param animationTime 
Detail::Skeleton Detail::ApplyBoneAnimation(const Skeleton& skeleton, const AnimationData& animation, float animationTime)
{
	Skeleton skeleton1 = skeleton;

	for (JointNode& joint : skeleton1.joints)
	{
		// 対象のjointのアニメーションがあれば、値の適用を行う
		if (auto it = animation.indices.find(joint.name); it != animation.indices.end())
		{
			const NodeAnimation& rootNodeAnimation = animation.nodes[it->second];
			joint.initialTransform.translate = CalculateValue(rootNodeAnimation.translate, animationTime);
			joint.initialTransform.rotate = CalculateValue(rootNodeAnimation.rotate, animationTime);
			joint.initialTransform.scale = CalculateValue(rootNodeAnimation.scale, animationTime);
		}
	}

	return skeleton1;
}

/// @brief スケルトンのフォワードキネマティクスを行う
/// @param skeleton 
/// @param pose 
void Detail::ForwardKinematics(Skeleton& skeleton, SkeletonPose& pose)
{
	// 全てのjointを更新する
	for (size_t i = 0; i < skeleton.joints.size(); ++i)
	{
		const JointNode& joint = skeleton.joints[i];
		const JointTransform& transform = pose.localTransforms[i];

		// ローカル行列
		Matrix4x4 localMatrix = Make3DAffineMatrix4x4(transform.scale, transform.rotate, transform.translate);

		// 親がいれば行列をかける
		if (joint.parent)
		{
			pose.skeletonSpaceMatrices[i] = localMatrix * pose.skeletonSpaceMatrices[*joint.parent];
		}
		else
		{
			// 親がいないので、ローカル行列とSkeletonSpaceMatrixは一致する
			pose.skeletonSpaceMatrices[i] = localMatrix;
		}
	}
}

/// @brief スケルトンのインバースキネマティクス
/// @param skeleton 
/// @param pose 
void Detail::InverseKinematics(Skeleton& skeleton, SkeletonPose& pose, const std::string& effectedBoneName, const Vector3& targetPosition)
{
	// 末端のジョイントのインデックス
	const int32_t kEffectedBoneIndex = skeleton.jointMap[effectedBoneName];

	// 反復回数
	const int32_t kMaxIteration = 5;

	// 許容誤差
	const float kThreshold = 0.01f;

	for (int i = 0; i < kMaxIteration; ++i)
	{
		// 末端のジョイントからたどるためのインデックス
		int32_t currentIndex = kEffectedBoneIndex;

		// 親がいなくなるまでループ
		while (true)
		{
			// 現在のジョイントと末端のジョイント
			JointNode& currentJoint = skeleton.joints[currentIndex];
			JointNode& effectedJoint = skeleton.joints[kEffectedBoneIndex];

			JointTransform& currentTransform = pose.localTransforms[currentIndex];
			JointTransform& effectedTransform = pose.localTransforms[kEffectedBoneIndex];

			const Matrix4x4& currentWorldMatrix = pose.skeletonSpaceMatrices[currentIndex];
			const Matrix4x4& effectedWorldMatrix = pose.skeletonSpaceMatrices[kEffectedBoneIndex];

			Vector3 currentPosition = Vector3(currentWorldMatrix.m[3][0], currentWorldMatrix.m[3][1], currentWorldMatrix.m[3][2]);
			Vector3 effectedPosition = Vector3(effectedWorldMatrix.m[3][0], effectedWorldMatrix.m[3][1], effectedWorldMatrix.m[3][2]);

			// ターゲットと末端の距離が十分に近ければ終了
			if ((targetPosition - effectedPosition).Length() < kThreshold)
				return;

			// 現在のジョイントから見た末端のジョイントとターゲットの方向
			Vector3 toEffected = (effectedPosition - currentPosition).Normalize();
			Vector3 toTarget = (targetPosition - currentPosition).Normalize();

			// 回転軸を求める
			float dot = Dot(toEffected, toTarget);
			dot = std::clamp(dot, -1.0f, 1.0f);
			float angle = std::acos(dot);
			Vector3 axis = Cross(toEffected, toTarget).Normalize();


			if (std::abs(angle) > 0.001f && axis.Length() > 0.0f)
			{
				Vector3 localAxis = axis;

				if (currentJoint.parent)
				{
					// 親の逆ワールド行列
					Matrix4x4 inverseParentWorldMatrix = pose.skeletonSpaceMatrices[*currentJoint.parent].Inverse();

					// 回転軸をローカル空間に変換して回転を適用する
					localAxis = TransformNormal(localAxis, inverseParentWorldMatrix).Normalize();
				}

				// 回転をクォータニオンに変換する
				Quaternion deltaRotation = ToQuaternion(angle, localAxis);

				// 回転を適用する
				currentTransform.rotate = currentTransform.rotate * deltaRotation;
				currentTransform.rotate = currentTransform.rotate.Normalize();

				// フォワードキネマティクスを再計算する
				ForwardKinematics(skeleton, pose);
			}

			// 親がいなければ終了
			if (!currentJoint.parent)
				break;

			currentIndex = *currentJoint.parent;
		}
	}
}