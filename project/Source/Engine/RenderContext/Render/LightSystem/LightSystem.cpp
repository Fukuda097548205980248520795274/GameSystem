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

	// 平行光源データ用の構造化バッファを作成
	directionalLightDataBuffer_ = std::make_unique<StructuredBufferResource<DirectionalLightDataForGPU>>(device, engine->GetRenderContext()->GetHeap(), kMaxNumLights, engine->GetMaxBufferCount());

	// ポイントライトデータ用の構造化バッファを作成
	pointLightDataBuffer_ = std::make_unique<StructuredBufferResource<PointLightDataForGPU>>(device, engine->GetRenderContext()->GetHeap(), kMaxNumLights, engine->GetMaxBufferCount());

	// スポットライトデータ用の構造化バッファを作成
	spotLightDataBuffer_ = std::make_unique<StructuredBufferResource<SpotLightDataForGPU>>(device, engine->GetRenderContext()->GetHeap(), kMaxNumLights, engine->GetMaxBufferCount());

	// タイルライトインデックス用のRW構造化バッファを作成
	int width = engine->GetClientWidth();
	int height = engine->GetClientHeight();

	// タイルの幅と高さを計算
	const int tileWidth =(width + kTileSize - 1) / kTileSize;
	const int tileHeight =(height + kTileSize - 1) / kTileSize;

	// タイルの総数を計算
	const int tileCount = tileWidth * tileHeight;

	// タイルごとの要素数を計算
	const int elementsPerTile =256 * 2 + 2;

	// RW構造化バッファを作成
	tileLightIndexBuffer_ = std::make_unique<RWStructuredBufferResource<uint32_t>>(device, commandList, engine->GetRenderContext()->GetHeap(), tileCount * elementsPerTile, engine->GetMaxBufferCount());
	tileLightIndexBuffer_->AllBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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

	// タイルの幅と高さを計算
	const int tileWidth = (width + kTileSize - 1) / kTileSize;
	const int tileHeight = (height + kTileSize - 1) / kTileSize;

	// タイルの総数を計算
	const int tileCount = tileWidth * tileHeight;

	// タイルごとの要素数を計算
	const int elementsPerTile = 256 * 2 + 2;
	
	// RW構造化バッファをリサイズ
	tileLightIndexBuffer_->Resize(device, commandList, tileCount * elementsPerTile);
	tileLightIndexBuffer_->AllBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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
	tileLightIndexBuffer_->Barrier(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, frameIndex);

	/*---------------------------
		ライトのデータを設定する
	---------------------------*/

	// ライト数
	int numDirectionalLights = 0;
	int numPointLights = 0;
	int numSpotLights = 0;

	// ライト数データを設定する
	registry_->ForEachEntityWithAny<DirectionalLightComponent, PointLightComponent, SpotLightComponent>(
		[&](Entity entity, DirectionalLightComponent* directionalLight, PointLightComponent* pointLight, SpotLightComponent* spotLight)
		{
			// 平行光源
			if (directionalLight && directionalLight->isEnabled)
			{
				if (numDirectionalLights < kMaxNumLights)
				{
					auto& lightData = directionalLightDataBuffer_->GetData(frameIndex)[numDirectionalLights];
					lightData.direction = directionalLight->direction;
					lightData.color = Vector4(directionalLight->color.x, directionalLight->color.y, directionalLight->color.z, 1.0f);
					lightData.intensity = directionalLight->intensity;
					numDirectionalLights++;
				}
			}

			// ポイントライト
			if (pointLight && pointLight->isEnabled)
			{
				if (numPointLights < kMaxNumLights)
				{
					auto& lightData = pointLightDataBuffer_->GetData(frameIndex)[numPointLights];
					lightData.position = pointLight->position;
					lightData.color = Vector4(pointLight->color.x, pointLight->color.y, pointLight->color.z, 1.0f);
					lightData.intensity = pointLight->intensity;
					lightData.radius = pointLight->radius;
					lightData.decay = pointLight->decay;
					numPointLights++;
				}
			}

			// スポットライト
			if (spotLight && spotLight->isEnabled)
			{
				if (numSpotLights < kMaxNumLights)
				{
					auto& lightData = spotLightDataBuffer_->GetData(frameIndex)[numSpotLights];
					lightData.position = spotLight->position;
					lightData.direction = spotLight->direction;
					lightData.color = Vector4(spotLight->color.x, spotLight->color.y, spotLight->color.z, 1.0f);
					lightData.intensity = spotLight->intensity;
					lightData.distance = spotLight->distance;
					lightData.decay = spotLight->decay;
					lightData.cosAngle = spotLight->cosAngle;
					lightData.cosFalloffStart = spotLight->cosFalloffStart;
					numSpotLights++;
				}
			}
		});

	// ライト数データをGPUに転送する
	auto& numLightData = numLightDataBuffer_->GetData(frameIndex)[0];
	numLightData.numDirectionalLights = numDirectionalLights;
	numLightData.numPointLights = numPointLights;
	numLightData.numSpotLights = numSpotLights;

	// カメラデータをGPUに転送する
	auto cameraSystem = Engine::GetInstance()->GetRenderContext()->GetCameraSystem();
	if (cameraSystem)
	{
		auto& cameraData = cameraDataBuffer_->GetData(frameIndex)[0];
		Matrix4x4 cameraMatrix = cameraSystem->GetCameraMatrix();

		cameraData.nearZ = cameraSystem->GetNearZ();
		cameraData.farZ = cameraSystem->GetFarZ();
		cameraData.screenDimensions = Vector2(static_cast<float>(Engine::GetInstance()->GetClientWidth()), static_cast<float>(Engine::GetInstance()->GetClientHeight()));
		cameraData.worldPosition = Vector3(cameraMatrix.m[3][0], cameraMatrix.m[3][1], cameraMatrix.m[3][2]);
		cameraData.view = cameraSystem->GetViewMatrix();
		cameraData.projection = cameraSystem->GetProjectionMatrix();
		cameraData.inverseProjection = cameraSystem->GetProjectionMatrix().Inverse();
	}

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
	lightCullingDepthResource_->Barrier(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, frameIndex);


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
	lightCullingDepthResource_->Barrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE, frameIndex);


	// タイルライトインデックス用のRW構造化バッファにバリアを張る
	tileLightIndexBuffer_->Barrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, frameIndex);
}