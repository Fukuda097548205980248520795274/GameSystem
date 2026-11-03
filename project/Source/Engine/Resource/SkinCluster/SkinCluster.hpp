#pragma once
#include <span>
#include "Store/SkeletonStore/SkeletonData.hpp"
#include "Store/ModelStore/ModelData.hpp"
#include "RenderContext/DXHeap/DXHeap.hpp"

namespace Detail
{
	/// @brief スキンクラスター
	class SkinCluster
	{
	public:

		/// @brief コンストラクタ
		/// @param heap 
		/// @param device 
		/// @param meshData 
		/// @param meshBoneData 
		/// @param skeleton 
		SkinCluster(DXHeap* heap, ID3D12Device* device, const MeshData& meshData, const MeshBoneData& meshBoneData, const Skeleton& skeleton) { Initialize(heap, device, meshData, meshBoneData, skeleton); }

		/// @brief デストラクタ
		~SkinCluster();

		/// @brief 更新処理
		/// @param skeleton 
		void Update(const SkeletonPose& pose);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param wellRootParameterIndex 
		/// @param influenceRootParameterIndex 
		void Register(ID3D12GraphicsCommandList* commandList, UINT wellRootParameterIndex, UINT influenceRootParameterIndex);


	private:

		/// @brief 初期化
		/// @param heap 
		/// @param device 
		/// @param meshData 
		/// @param skeleton 
		void Initialize(DXHeap* heap, ID3D12Device* device, const MeshData& meshData, const MeshBoneData& meshBoneData, const Skeleton& skeleton);


		/// @brief バインド行列の逆行列
		std::vector<Matrix4x4> inverseBindPoseMatrices_;


	private:

		// インフルエンスリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource_;

		// インフルエンスのマッピングデータ
		std::span<VertexInfluence> mappedInfluence_;

		// インフルエンスSRVハンドル
		SRVDescriptorHandle influenceSrvHandle_;


	private:

		// パレットリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource_;

		// パレットのマッピングデータ
		std::span<WellForGPU> mappedPalette_;

		// パレットSRVハンドル
		SRVDescriptorHandle paletteSrvHandle_;


	private:

		// ヒープ
		DXHeap* heap_ = nullptr;

		// インフルエンスの最大数
		static constexpr uint32_t kNumMaxInfluence = 4;
	};
}