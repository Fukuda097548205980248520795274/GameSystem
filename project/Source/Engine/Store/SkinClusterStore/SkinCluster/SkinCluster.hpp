#pragma once
#include <span>
#include "Store/SkeletonStore/SkeletonData.hpp"
#include "Store/ModelStore/ModelData.hpp"
#include "RenderContext/DXHeap/DXHeap.hpp"
#include "Store/HandleData/HandleData.hpp"

#include "Resource/RWStructuredBufferResource/RWStructuredBufferResource.hpp"

namespace Detail
{
	// @brief スキンクラスター用のハンドル
	struct SkinClusterTag {};
	using SkinClusterHandle = Handle<SkinClusterTag>;

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
		/// @param maxBufferCount 
		SkinCluster(DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const Skeleton& skeleton, const MeshData& meshData, uint32_t maxBufferCount = 1) { Initialize(heap, device, commandList, skeleton, meshData, maxBufferCount); }

		/// @brief デストラクタ
		~SkinCluster();

		/// @brief 更新処理
		/// @param pose 
		/// @param inverseBindPoseMatrices 
		/// @param frameIndex 
		void Update(const SkeletonPose& pose, const std::vector<Matrix4x4>& inverseBindPoseMatrices, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param wellRootParameterIndex 
		/// @param influenceRootParameterIndex 
		/// @param frameIndex 
		void Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);



	private:

		/// @brief 初期化
		/// @param heap 
		/// @param device 
		/// @param meshData 
		/// @param meshBoneData
		/// @param skeleton 
		/// @param maxBufferCount 
		void Initialize(DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const Skeleton& skeleton, const MeshData& meshData, uint32_t maxBufferCount);

		/// @brief 最大バッファ数
		uint32_t maxBufferCount_ = 1;


	private:

		// パレットリソース（動的データのためマルチバッファ化）
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> paletteResource_;

		// パレットのマッピングデータ
		std::vector<std::span<WellForGPU>> mappedPalette_;

		// パレットSRVハンドル
		std::vector<SRVDescriptorHandle> paletteSrvHandle_;


	private:


		// 出力頂点リソース（動的データのためマルチバッファ化）
		std::unique_ptr<RWStructuredBufferResource<ModelVertexData>> outputVertexResource_;


	private:

		// ヒープ
		DXHeap* heap_ = nullptr;
	};
}