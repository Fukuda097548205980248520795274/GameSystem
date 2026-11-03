#include "SkinCluster.hpp"
#include "Func/ResourceFunc/ResourceFunc.hpp"
#include <algorithm>
#include <cassert>

/// @brief デストラクタ
Detail::SkinCluster::~SkinCluster()
{
	if (heap_)
	{
		// ヒープからSRVハンドルを解放する
		heap_->FreeSrvDescriptorHandle(paletteSrvHandle_);
		heap_->FreeSrvDescriptorHandle(influenceSrvHandle_);
	}
}

/// @brief 初期化
/// @param heap 
/// @param device 
/// @param meshData 
/// @param meshBoneData 
/// @param skeleton 
void Detail::SkinCluster::Initialize(DXHeap* heap, ID3D12Device* device, const MeshData& meshData, const MeshBoneData& meshBoneData, const Skeleton& skeleton)
{
	// nullptr チェック
	assert(heap);
	heap_ = heap;


	/*------------------------------
		Palette用のリソースを確保
	------------------------------*/

	paletteResource_ = CreateBufferResource(device, sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPaletteData = nullptr;
	paletteResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedPaletteData));

	// spanを使ってアクセスするようにする
	mappedPalette_ = { mappedPaletteData, skeleton.joints.size() };
	paletteSrvHandle_ = heap->GetSrvDescriptorHandle();

	// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = UINT(skeleton.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);

	// ビューの生成
	device->CreateShaderResourceView(paletteResource_.Get(), &paletteSrvDesc, paletteSrvHandle_.cpuHandle);


	/*-------------------------------
		Influence用のリソースを確保
	-------------------------------*/

	// 頂点数を取得する
	uint32_t numVertices = static_cast<uint32_t>(meshData.vertexBufferResource->GetNumVertex());

	// Influence用のリソースを作成する
	influenceResource_ = CreateBufferResource(device, sizeof(VertexInfluence) * numVertices);
	VertexInfluence* mappedInfluenceData = nullptr;
	influenceResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluenceData));

	// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC influenceSrvDesc{};
	influenceSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	influenceSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	influenceSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	influenceSrvDesc.Buffer.FirstElement = 0;
	influenceSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	influenceSrvDesc.Buffer.NumElements = UINT(numVertices);
	influenceSrvDesc.Buffer.StructureByteStride = sizeof(VertexInfluence);

	// 0埋め
	std::memset(mappedInfluenceData, 0, sizeof(VertexInfluence) * numVertices);
	mappedInfluence_ = { mappedInfluenceData, numVertices };

	// SRVハンドルを取得する
	influenceSrvHandle_ = heap_->GetSrvDescriptorHandle();

	// ビューの生成
	device->CreateShaderResourceView(influenceResource_.Get(), &influenceSrvDesc, influenceSrvHandle_.cpuHandle);


	/*----------------------------------------------
		SkinClusterの情報を解析してInfluenceを格納する
	----------------------------------------------*/

	// バインド行列の逆行列を初期化する
	inverseBindPoseMatrices_.resize(skeleton.joints.size());
	std::generate(inverseBindPoseMatrices_.begin(), inverseBindPoseMatrices_.end(), MakeIdentityMatrix4x4);

	// jointWeightの情報を解析して、各頂点に対するInfluenceを設定する
	for (const auto& jointWeight : meshBoneData.jointWeights)
	{
		// skeletonに対象となるjointが含まれているか判断する
		auto it = skeleton.jointMap.find(jointWeight.first);

		// 存在しない名前のJointは後回し
		if (it == skeleton.jointMap.end())
			continue;

		// 該当のIndexのInverseBindPoseMatrixを代入する
		inverseBindPoseMatrices_[(*it).second] = jointWeight.second.inverseBindPoseMatrix;

		for (const auto& vertexWeight : jointWeight.second.vertexWeights)
		{
			// 該当のvertexIndexのInfluence情報を参照しておく
			auto& currentInfluence = mappedInfluence_[vertexWeight.vertexIndex];

			// 空いているところ (Weight == 0.0f) に入れる
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index)
			{
				if (currentInfluence.weights[index] == 0.0f)
				{
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}
}

/// @brief 更新処理
/// @param skeleton 
void Detail::SkinCluster::Update(const SkeletonPose& pose)
{
	for (size_t jointIndex = 0; jointIndex < pose.skeletonSpaceMatrices.size(); ++jointIndex)
	{
		assert(jointIndex < inverseBindPoseMatrices_.size());

		// バインドポーズ逆行列 × 現在のスケルトン空間行列
		mappedPalette_[jointIndex].skeletonSpaceMatrix =
			inverseBindPoseMatrices_[jointIndex] * pose.skeletonSpaceMatrices[jointIndex];

		mappedPalette_[jointIndex].skeletonSpaceInverseTransposeMatrix =
			mappedPalette_[jointIndex].skeletonSpaceMatrix.Inverse().Transpose();
	}
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param wellRootParameterIndex 
/// @param influenceRootParameterIndex 
void Detail::SkinCluster::Register(ID3D12GraphicsCommandList* commandList, UINT wellRootParameterIndex, UINT influenceRootParameterIndex)
{
	commandList->SetComputeRootDescriptorTable(wellRootParameterIndex, paletteSrvHandle_.gpuHandle);
	commandList->SetComputeRootDescriptorTable(influenceRootParameterIndex, influenceSrvHandle_.gpuHandle);
}