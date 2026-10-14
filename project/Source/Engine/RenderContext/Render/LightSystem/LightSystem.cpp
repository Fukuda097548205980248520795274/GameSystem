#include "LightSystem.h"
#include "Engine.h"

/// @brief 初期化
/// @param device 
/// @param commandList 
/// @param compiler 
void Detail::LightSystem::Initialize(ID3D12Device* device,ID3D12GraphicsCommandList* commandList, ShaderCompiler* compiler)
{
	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();
	assert(engine);

	// レジストリを取得
	registry_ = engine->GetRegistryECS();

	// ライトカリングPSOを初期化
	lightCullingPso_ = std::make_unique<LightCullingPSO>(device, compiler);


	// カメラデータ用の定数バッファを作成
	cameraDataBuffer_ = std::make_unique<ConstantBufferResource<LightCullingCameraDataForGPU>>(device, engine->GetMaxBufferCount());

	// ライト数データ用の構造化バッファを作成
	numLightDataBuffer_ = std::make_unique<ConstantBufferResource<NumLightDataForGPU>>(device, engine->GetMaxBufferCount());

	// ポイントライトデータ用の構造化バッファを作成
	pointLightDataBuffer_ = std::make_unique<StructuredBufferResource<PointLightDataForGPU>>(device, engine->GetRenderContext()->GetHeap(), kMaxNumLights, engine->GetFrameIndex());

	// スポットライトデータ用の構造化バッファを作成
	spotLightDataBuffer_ = std::make_unique<StructuredBufferResource<SpotLightDataForGPU>>(device, engine->GetRenderContext()->GetHeap(), kMaxNumLights, engine->GetFrameIndex());

	// タイルライトインデックス用のRW構造化バッファを作成
	int width = engine->GetClientWidth() / kTileSize;
	int height = engine->GetClientHeight() / kTileSize;
	tileLightIndexBuffer_ = std::make_unique<RWStructuredBufferResource<uint32_t>>(device, commandList, engine->GetRenderContext()->GetHeap(), width * height, engine->GetFrameIndex());
}