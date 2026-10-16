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

	// ライトカリング用の深度リソースを作成
	lightCullingDepthResource_ = std::make_unique<DepthResource>(device, engine->GetClientWidth(), engine->GetClientHeight(), engine->GetRenderContext()->GetHeap());


	// カメラデータ用の定数バッファを作成
	cameraDataBuffer_ = std::make_unique<ConstantBufferResource<LightCullingCameraDataForGPU>>(device, engine->GetMaxBufferCount());

	// ライト数データ用の構造化バッファを作成
	numLightDataBuffer_ = std::make_unique<ConstantBufferResource<NumLightDataForGPU>>(device, engine->GetMaxBufferCount());

	// ポイントライトデータ用の構造化バッファを作成
	pointLightDataBuffer_ = std::make_unique<StructuredBufferResource<PointLightDataForGPU>>(device, engine->GetRenderContext()->GetHeap(), kMaxNumLights, engine->GetMaxBufferCount());

	// スポットライトデータ用の構造化バッファを作成
	spotLightDataBuffer_ = std::make_unique<StructuredBufferResource<SpotLightDataForGPU>>(device, engine->GetRenderContext()->GetHeap(), kMaxNumLights, engine->GetMaxBufferCount());

	// タイルライトインデックス用のRW構造化バッファを作成
	int tileWidth = engine->GetClientWidth() / kTileSize;
	int tileHeight = engine->GetClientHeight() / kTileSize;
	tileLightIndexBuffer_ = std::make_unique<RWStructuredBufferResource<uint32_t>>(device, commandList, engine->GetRenderContext()->GetHeap(), tileWidth * tileHeight, engine->GetMaxBufferCount());
}

/// @brief リサイズ
/// @param device 
/// @param commandList 
/// @param width 
/// @param height 
void Detail::LightSystem::Resize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, int32_t width, int32_t height)
{
	// ライトカリング用の深度リソースをリサイズ
	lightCullingDepthResource_->Resize(device, width, height);

	// カメラデータ用の定数バッファをリサイズ
	int tileWidth = width / kTileSize;
	int tileHeight = height / kTileSize;
	tileLightIndexBuffer_->Resize(device, commandList, tileWidth * tileHeight);
}

/// @brief 平行光源のエンティティを作成する
/// @param direction 
/// @param color 
/// @param intensity 
/// @return 
Entity Detail::LightSystem::CreateDirectionalLightEntity(const Vector3& direction, const Vector4& color, float intensity)
{
	Entity entity = registry_->CreateEntity();

	DirectionalLightComponent directionalLight;
	directionalLight.direction = direction.Normalize();
	directionalLight.color = Vector3(color.x, color.y, color.z);
	directionalLight.intensity = intensity;
	registry_->AddComponent(entity, directionalLight);

	return entity;	
}

/// @brief ポイントライトのエンティティを作成する
/// @param position 
/// @param color 
/// @param intensity 
/// @param radius 
/// @param decay 
/// @return 
Entity Detail::LightSystem::CreatePointLightEntity(const Vector3& position, const Vector4& color, float intensity, float radius, float decay)
{
	Entity entity = registry_->CreateEntity();

	PointLightComponent pointLight;
	pointLight.position = position;
	pointLight.color = Vector3(color.x, color.y, color.z);
	pointLight.intensity = intensity;
	pointLight.radius = radius;
	pointLight.decay = decay;
	registry_->AddComponent(entity, pointLight);

	return entity;
}

/// @brief スポットライトのエンティティを作成する
/// @param position 
/// @param direction 
/// @param color 
/// @param intensity 
/// @param radius 
/// @param decay 
/// @param cosAngle 
/// @param cosFalloffStart 
/// @return 
Entity Detail::LightSystem::CreateSpotLightEntity(const Vector3& position, const Vector3& direction, const Vector4& color,
	float intensity, float radius, float decay, float cosAngle, float cosFalloffStart)
{
	Entity entity = registry_->CreateEntity();

	SpotLightComponent spotLight;
	spotLight.position = position;
	spotLight.direction = direction.Normalize();
	spotLight.color = Vector3(color.x, color.y, color.z);
	spotLight.intensity = intensity;
	spotLight.distance = radius;
	spotLight.decay = decay;
	spotLight.cosAngle = cosAngle;
	spotLight.cosFalloffStart = cosFalloffStart;
	registry_->AddComponent(entity, spotLight);

	return entity;
}

/// @brief ライトカリング
/// @param commandList 
/// @param frameIndex 
void Detail::LightSystem::LightCulling(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex, RenderSystem* renderSystem)
{
	/*---------------------------
		ライトのデータを設定する
	---------------------------*/

	


	/*---------------------------------------
	    描画するオブジェクトの深度値を取得する
	---------------------------------------*/

	// 深度値をクリアする
	lightCullingDepthResource_->ClearDepthStencil(commandList, frameIndex);

	// 深度値を取得するためのレンダーターゲットを設定する
	lightCullingDepthResource_->SetRenderTargetView(commandList, frameIndex);

	// 深度値を取得するためのレンダーパスを実行する
	renderSystem->ExecuteDepthPrePass(commandList, frameIndex);


	/*---------------------------------------
		深度値をもとに、ライトカリングを行う
	---------------------------------------*/

	// ライトカリング用の深度リソースにバリアを張る
	lightCullingDepthResource_->Barrier(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, frameIndex);


	// ライトカリングPSOを登録する
	lightCullingPso_->Register(commandList);

	// カメラデータ用の定数バッファを登録する
	cameraDataBuffer_->RegisterCompute(commandList, 0, frameIndex);

	// ライト数データ用の構造化バッファを登録する
	numLightDataBuffer_->RegisterCompute(commandList, 1, frameIndex);

	// ポイントライトデータ用の構造化バッファを登録する
	pointLightDataBuffer_->RegisterCompute(commandList, 2, frameIndex);

	// スポットライトデータ用の構造化バッファを登録する
	spotLightDataBuffer_->RegisterCompute(commandList, 3, frameIndex);

	// ライトカリング用の深度リソースを登録する
	lightCullingDepthResource_->RegisterCompute(commandList, 4, frameIndex);

	// タイルライトインデックス用のRW構造化バッファを登録する
	tileLightIndexBuffer_->RegisterComputeUAV(commandList, 5, frameIndex);

	// ライトカリングを実行する
	commandList->Dispatch((Engine::GetInstance()->GetClientWidth() + kTileSize - 1) / kTileSize, (Engine::GetInstance()->GetClientHeight() + kTileSize - 1) / kTileSize, 1);


	// ライトカリング用の深度リソースにバリアを張る
	lightCullingDepthResource_->Barrier(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE, frameIndex);
}