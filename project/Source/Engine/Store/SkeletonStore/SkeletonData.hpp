#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <array>
#include "Matrix/Matrix4x4/Matrix4x4.hpp"
#include "Quaternion/Quaternion.hpp"
#include "Vector/Vector3/Vector3.hpp"
#include "Store/HandleData/HandleData.hpp"

namespace Detail
{
    // スケルトン用のハンドル
    struct SkeletonTag {};
    using SkeletonHandle = Handle<SkeletonTag>;

    // ジョイントのトランスフォーム情報
    struct JointTransform
    {
        Vector3 scale{ 1.0f, 1.0f, 1.0f };
        Quaternion rotate{ 0.0f, 0.0f, 0.0f, 1.0f };
        Vector3 translate{ 0.0f, 0.0f, 0.0f };
    };

    // 静的なジョイント定義（モデル共有リソース）
    struct JointNode
    {
        std::string name;
        int32_t index;
        std::optional<int32_t> parent;
        std::vector<int32_t> children;
        JointTransform initialTransform; // 初期ポーズ
    };

    // 静的なスケルトン構造
    struct Skeleton
    {
		// ジョイントのリスト
        int32_t rootIndex = 0;

		// ジョイントのリスト
        std::vector<JointNode> joints;

		// ジョイント名からインデックスへのマップ
        std::unordered_map<std::string, int32_t> jointMap;
    };

    // 動的なスケルトン姿勢（キャラクター単位で保持するインスタンス）
    struct SkeletonPose
    {
        // 毎フレーム変化するローカルのトランスフォーム
        std::vector<JointTransform> localTransforms;

        // 計算済みの変換行列（SkinCluster更新用）
        std::vector<Matrix4x4> skeletonSpaceMatrices;

		// @brief 初期化する
        void Initialize(const Skeleton& skeleton)
        {
            size_t count = skeleton.joints.size();
            localTransforms.resize(count);
            skeletonSpaceMatrices.resize(count);
            for (size_t i = 0; i < count; ++i)
            {
                localTransforms[i] = skeleton.joints[i].initialTransform;
                skeletonSpaceMatrices[i] = MakeIdentityMatrix4x4();
            }
        }

		// @brief スケルトン空間行列を更新する
		void UpdateSkeletonSpaceMatrices(const Skeleton& skeleton)
		{
			for (size_t i = 0; i < skeleton.joints.size(); ++i)
			{
				const auto& joint = skeleton.joints[i];
				Matrix4x4 localMatrix = Make3DAffineMatrix4x4(localTransforms[i].scale, localTransforms[i].rotate, localTransforms[i].translate);
				if (joint.parent.has_value())
				{
					int32_t parentIndex = joint.parent.value();
					skeletonSpaceMatrices[i] = localMatrix * skeletonSpaceMatrices[parentIndex];
				}
				else
				{
					skeletonSpaceMatrices[i] = localMatrix;
				}
			}
		}
    };

    // 頂点ウェイトデータ
    struct VertexWeightData
    {
		// ウェイト
        float weight;

		// 頂点インデックス
        uint32_t vertexIndex;
    };

    // ジョイントウェイトデータ
    struct JointWeightData
    {
		// ジョイントインデックス
        int32_t jointIndex;

		// ジョイントに関連付けられた頂点ウェイトデータ
        std::vector<VertexWeightData> vertexWeights;

		// ジョイントのバインド行列の逆行列
        Matrix4x4 inverseBindPoseMatrix;
    };

    /// @brief 頂点インフルエンス
    struct VertexInfluence
    {
		// ウェイト
        std::array<float, 4> weights;

		// ジョイントインデックス
        std::array<int32_t, 4> jointIndices;
    };

    /// @brief スキニングデータ
    struct SkinningData
    {
		// ジョイント数
        uint32_t jointCount;

		// インフルエンス
        std::vector<VertexInfluence> influence;
    };


    // GPUに送るウェル
    struct WellForGPU
    {
        // 位置用
        Matrix4x4 skeletonSpaceMatrix;

        // 法線用
        Matrix4x4 skeletonSpaceInverseTransposeMatrix;
    };


    /// @brief メッシュボーンデータ
    struct MeshBoneData
    {
        // ジョイントウェイト
        std::unordered_map<std::string, JointWeightData> jointWeights;
    };

    /// @brief モデルボーンデータ
    struct ModelBoneData
    {
        // メッシュボーンデータ
        std::vector<MeshBoneData> meshes;

        // メッシュボーンインデックス
        std::unordered_map<std::string, int32_t> meshIndices;
    };

    // 静的なスケルトンデータ（モデル共有リソース）
    struct SkeletonData
    {
        // 静的なスケルトン構造
        Skeleton skeleton;

        // スキニング用データ（バインドポーズ逆行列や頂点ウェイト情報）
        ModelBoneData boneData;

        // 元のファイルパス
        std::string filePath;
    };
}