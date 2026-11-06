#include "SkinningInstance.hpp"
#include "Engine.hpp"

/// @brief デストラクタ
Detail::SkinningInstance::~SkinningInstance()
{
	if (heap_)
	{
		// ヒープからSRVハンドルを解放する
		heap_->FreeSrvDescriptorHandle(influenceSrvHandle_);
	}
}

void Detail::SkinningInstance::Initialize(DXHeap* heap, ID3D12Device* device, const Skeleton& skeleton, const MeshData& meshData, const MeshBoneData& meshBoneData)
{
	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();
	assert(engine);

	assert(heap);
	heap_ = heap;

	// SkeletonStoreを取得する
	auto skeletonStore = engine->GetRenderContext()->GetSkeletonStore();
	assert(skeletonStore);


	// 頂点数を取得する
	uint32_t numVertices = static_cast<uint32_t>(meshData.vertexBufferResource->GetNumVertex());

	// メッシュデータの頂点データを取得する
	auto vertexData = meshData.vertexBufferResource->GetData(0);


	// 入力頂点リソースを作成する
	inputVertexResource_ = std::make_unique<StructuredBufferResource<ModelVertexData>>(device, heap_, meshData.vertexBufferResource->GetNumVertex(), 1);
	auto inputData = inputVertexResource_->GetData(0);
	std::memcpy(inputData, vertexData, sizeof(ModelVertexData) * meshData.vertexBufferResource->GetNumVertex());

	// 入力頂点数リソースを作成する
	numVertexResource_ = std::make_unique<ConstantBufferResource<uint32_t>>(device, 1);
	auto numVertexData = numVertexResource_->GetData(0);
	*numVertexData = numVertices;



	/*-------------------------------
		Influence用のリソースを確保
	-------------------------------*/

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
		// skeletonに対象となるjointが含まれているか判断するs
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

/// @brief コマンドリストに登録する
/// @param commandList 
void Detail::SkinningInstance::Register(ID3D12GraphicsCommandList* commandList)
{
	// 入力頂点リソースを登録する
	inputVertexResource_->RegisterCompute(commandList, 1, 0);

	// インフルエンスSRVを登録する
	commandList->SetComputeRootDescriptorTable(2, influenceSrvHandle_.gpuHandle);

	// 頂点数リソースを登録する
	numVertexResource_->RegisterCompute(commandList, 4, 0);
}