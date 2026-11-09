#include "SkinCluster.hpp"
#include "Func/ResourceFunc/ResourceFunc.hpp"
#include "Engine.hpp"
#include <algorithm>
#include <cassert>

/// @brief デストラクタ
Detail::SkinCluster::~SkinCluster()
{
	if (heap_)
	{
		// ヒープからSRVハンドルを解放する
		for (uint32_t i = 0; i < maxBufferCount_; ++i)
			heap_->FreeSrvDescriptorHandle(paletteSrvHandle_[i]);
	}
}

/// @brief 初期化
/// @param heap 
/// @param device 
/// @param meshData 
/// @param meshBoneData 
/// @param skeleton 
/// @param maxBufferCount 
void Detail::SkinCluster::Initialize(DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const Skeleton& skeleton, const MeshData& meshData, uint32_t maxBufferCount)
{
	// nullptr チェック
	assert(heap);
	heap_ = heap;
	maxBufferCount_ = maxBufferCount;

	// メッシュデータの妥当性を確認する
	if (!(meshData.indexBufferResource && meshData.vertexBufferResource))throw std::runtime_error("Mesh data is invalid.");

	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();
	assert(engine);
    
	auto  placedResourceAllocator = engine->GetRenderContext()->GetPlacedResourceAllocator();
	assert(placedResourceAllocator);   

	// 出力頂点リソースを作成する
	outputVertexResource_ = std::make_unique<RWStructuredBufferResource<ModelVertexData>>(device, commandList, heap_, placedResourceAllocator, meshData.vertexBufferResource->GetNumVertex(), maxBufferCount_);

	// 出力頂点リソースの状態を UAV から VERTEX_AND_CONSTANT_BUFFER に変更する
	outputVertexResource_->AllBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);


	/*------------------------------
		Palette用のリソースを確保
	------------------------------*/

	paletteResource_.resize(maxBufferCount_);
	mappedPalette_.resize(maxBufferCount_);
	paletteSrvHandle_.resize(maxBufferCount_);
	outputVertexBufferViews_.resize(maxBufferCount_);

	for (uint32_t i = 0; i < maxBufferCount_; ++i)
	{
		// 出力頂点バッファビューを設定する
		outputVertexBufferViews_[i].BufferLocation = outputVertexResource_->Get(i)->GetGPUVirtualAddress();
		outputVertexBufferViews_[i].SizeInBytes = UINT(meshData.vertexBufferResource->GetNumVertex()) * sizeof(ModelVertexData);
		outputVertexBufferViews_[i].StrideInBytes = sizeof(ModelVertexData);


		// Palette用のリソースを作成する
		paletteResource_[i] = CreateBufferResource(device, sizeof(WellForGPU) * skeleton.joints.size());
		WellForGPU* mappedPaletteData = nullptr;
		paletteResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedPaletteData));

		// spanを使ってアクセスするようにする
		mappedPalette_[i] = { mappedPaletteData, skeleton.joints.size() };
		paletteSrvHandle_[i] = heap->GetSrvDescriptorHandle();

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
		device->CreateShaderResourceView(paletteResource_[i].Get(), &paletteSrvDesc, paletteSrvHandle_[i].cpuHandle);
	}
}

/// @brief 更新処理
/// @param skeleton 
void Detail::SkinCluster::Update(const SkeletonPose& pose, const std::vector<Matrix4x4>& inverseBindPoseMatrices, uint32_t frameIndex)
{
	for (size_t jointIndex = 0; jointIndex < pose.skeletonSpaceMatrices.size(); ++jointIndex)
	{
		assert(jointIndex < inverseBindPoseMatrices.size());

		// バインドポーズ逆行列 × 現在のスケルトン空間行列
		mappedPalette_[frameIndex][jointIndex].skeletonSpaceMatrix =
			inverseBindPoseMatrices[jointIndex] * pose.skeletonSpaceMatrices[jointIndex];

		mappedPalette_[frameIndex][jointIndex].skeletonSpaceInverseTransposeMatrix =
			mappedPalette_[frameIndex][jointIndex].skeletonSpaceMatrix.Inverse().Transpose();
	}
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param wellRootParameterIndex 
/// @param influenceRootParameterIndex 
/// @param frameIndex 
void Detail::SkinCluster::Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	// パレットSRVハンドルを設定する
	commandList->SetComputeRootDescriptorTable(0, paletteSrvHandle_[frameIndex].gpuHandle);

	// 出力用の頂点リソースをUAVとして登録する
	outputVertexResource_->RegisterComputeUAV(commandList, 3, frameIndex);
}

/// @brief コマンドリストに登録する（頂点用）
/// @param commandList 
/// @param frameIndex 
void Detail::SkinCluster::RegisterVertex(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	// 出力頂点リソースを頂点バッファとして登録する
	commandList->IASetVertexBuffers(0, 1, &outputVertexBufferViews_[frameIndex]);
}
