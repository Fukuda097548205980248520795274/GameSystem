#pragma once
#include <span>
#include "Store/SkeletonStore/SkeletonData.hpp"
#include "Store/ModelStore/ModelData.hpp"
#include "RenderContext/DXHeap/DXHeap.hpp"
#include "Store/HandleData/HandleData.hpp"

#include "Resource/ConstantBufferResource/ConstantBufferResource.hpp"
#include "Resource/StructuredBufferResource/StructuredBufferResource.hpp"

namespace Detail
{
	// スキニングインスタンス用のハンドル
	struct SkinningInstanceTag {};
	using SkinningInstanceHandle = Handle<SkinningInstanceTag>;

	class SkinningInstance
	{
	public:

		/// @brief コンストラクタ
		/// @param heap 
		/// @param device 
		/// @param handle 
		/// @param meshData 
		/// @param meshBoneData 
		SkinningInstance(DXHeap* heap, ID3D12Device* device, const Skeleton& skeleton, const MeshData& meshData, const MeshBoneData& meshBoneData) { Initialize(heap, device, skeleton, meshData, meshBoneData); }

		/// @brief デストラクタ
		~SkinningInstance();

		/// @brief バインド行列の逆行列を取得する
		/// @return 
		std::vector<Matrix4x4> GetInverseBindPoseMatrices() const { return inverseBindPoseMatrices_; }

		/// @brief コマンドリストに登録する
		/// @param commandList 
		void Register(ID3D12GraphicsCommandList* commandList);


	private:

		/// @brief 初期化
		/// @param heap 
		/// @param device 
		/// @param handle 
		/// @param meshData 
		/// @param meshBoneData 
		void Initialize(DXHeap* heap, ID3D12Device* device, const Skeleton& skeleton, const MeshData& meshData, const MeshBoneData& meshBoneData);

		/// @brief バインド行列の逆行列
		std::vector<Matrix4x4> inverseBindPoseMatrices_;

	private:

		// インフルエンスリソース（静的データのため1つで共有）
		Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource_;

		// インフルエンスのマッピングデータ
		std::span<VertexInfluence> mappedInfluence_;

		// インフルエンスSRVハンドル
		SRVDescriptorHandle influenceSrvHandle_;


	private:

		// 入力頂点リソース（静的データのため1つで共有）
		std::unique_ptr<StructuredBufferResource<ModelVertexData>> inputVertexResource_;

		// 入力頂点数リソース（静的データのため1つで共有）
		std::unique_ptr<ConstantBufferResource<uint32_t>> numVertexResource_;


	private:


		// ヒープ
		DXHeap* heap_ = nullptr;

		// インフルエンスの最大数
		static constexpr uint32_t kNumMaxInfluence = 4;
	};
}