#include "SkyboxSystem.hpp"
#include "Engine.hpp"

/// @brief 初期化
void Detail::SkyboxSystem::Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler)
{
	// nullptrチェック
	assert(device);
	assert(shaderCompiler);

	// Engineのインスタンスを取得する
	auto  engine = Engine::GetInstance();
	assert(engine);

	// ECSレジストリを取得する
	auto registry = engine->GetRegistryECS();
	assert(registry);
	registry_ = registry;

	// SkyboxPSOを初期化する
	pso_ = std::make_unique<SkyboxPSO>(device, shaderCompiler);


	// インデックスリソースの生成と初期化
	indexResource_ = std::make_unique<IndexBufferResource>(device, 36, 1);
	auto indexData = indexResource_->GetData(0);

	// 番号を与える
	for (int32_t i = 0; i < 6; ++i)
	{
		int32_t startIndex = i * 6;
		int32_t index = i * 4;

		indexData[startIndex] = index;
		indexData[startIndex + 1] = index + 1;
		indexData[startIndex + 2] = index + 2;
		indexData[startIndex + 3] = index + 1;
		indexData[startIndex + 4] = index + 3;
		indexData[startIndex + 5] = index + 2;
	}



	// 頂点リソースの生成と初期化
	vertexResource_ = std::make_unique<VertexBufferResource<Vector4>>(device, 24, 1);
	auto vertexData = vertexResource_->GetData(0);

	// ローカル座標を与える
	vertexData[0] = Vector4(-1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[1] = Vector4(-1.0f, 1.0f, -1.0f, 1.0f);
	vertexData[2] = Vector4(1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[3] = Vector4(1.0f, 1.0f, -1.0f, 1.0f);

	vertexData[4] = Vector4(1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[5] = Vector4(1.0f, 1.0f, -1.0f, 1.0f);
	vertexData[6] = Vector4(1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[7] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	
	vertexData[8] = Vector4(1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[9] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	vertexData[10] = Vector4(-1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[11] = Vector4(-1.0f, 1.0f, 1.0f, 1.0f);
	
	vertexData[12] = Vector4(-1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[13] = Vector4(-1.0f, 1.0f, 1.0f, 1.0f);
	vertexData[14] = Vector4(-1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[15] = Vector4(-1.0f, 1.0f, -1.0f, 1.0f);
	
	vertexData[16] = Vector4(-1.0f, 1.0f, -1.0f, 1.0f);
	vertexData[17] = Vector4(-1.0f, 1.0f, 1.0f, 1.0f);
	vertexData[18] = Vector4(1.0f, 1.0f, -1.0f, 1.0f);
	vertexData[19] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	vertexData[20] = Vector4(1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[21] = Vector4(1.0f, -1.0f, 1.0f, 1.0f);
	vertexData[22] = Vector4(-1.0f, -1.0f, -1.0f, 1.0f);
	vertexData[23] = Vector4(-1.0f, -1.0f, 1.0f, 1.0f);


	int maxBufferCount = engine->GetMaxBufferCount();
	assert(maxBufferCount > 0);

	// カラーバッファの生成
	colorBuffer_ = std::make_unique<ConstantBufferResource<Vector4>>(device, maxBufferCount);

	// 変換行列バッファの生成
	transformBuffer_ = std::make_unique<ConstantBufferResource<Matrix4x4>>(device, maxBufferCount);
}

/// @brief エンティティを作成する
/// @return 
Entity Detail::SkyboxSystem::CreateSkyboxEntity()
{
	auto engine = Engine::GetInstance();
	assert(engine);

	auto renderContext = engine->GetRenderContext();
	assert(renderContext);

	auto device = renderContext->GetCore()->GetDevice();
	assert(device);

	auto textureStore = renderContext->GetTextureStore();
	assert(textureStore);

	int32_t maxBufferCount = engine->GetMaxBufferCount();
	assert(maxBufferCount > 0);


	// エンティティを作成する
	Entity entity = registry_->CreateEntity();

	// SkyboxComponentを追加する
	SkyboxComponent skyboxComp;
	registry_->AddComponent(entity, skyboxComp);

	// CubemapComponentを追加する
	CubemapComponent cubemapComp(textureStore->GetDefaultCubemapHandle());
	registry_->AddComponent(entity, cubemapComp);

	return entity;
}

/// @brief すべての3D描画を実行する
/// @param commandList 
void Detail::SkyboxSystem::Execute(ID3D12GraphicsCommandList* commandList)
{
	// Engineのインスタンスを取得する
	auto engine = Engine::GetInstance();
	if (!engine) return;

	// テクスチャストアを取得する
	auto textureStore = engine->GetRenderContext()->GetTextureStore();
	if (!textureStore) return;

	// カメラシステムを取得する
	auto cameraSystem = engine->GetRenderContext()->GetCameraSystem();
	if (!cameraSystem) return;


	// SkyboxComponentの配列を取得する
	auto* passArray = registry_->GetComponentArray<SkyboxComponent>();
	const auto& entities = passArray->GetEntities();

	// 今のフレームインデックスを取得する
	int32_t frameIndex = engine->GetFrameIndex();

	// 選択されたエンティティを取得する
	Entity selectedEntity = kNullEntity;
	for (Entity entity : entities)
	{
		auto skybox = registry_->GetComponent<SkyboxComponent>(entity);
		
		if (skybox && skybox->isEnabled)
		{
			selectedEntity = entity;
			break;
		}
	}

	// 選択されたエンティティがない場合は何もしない
	if (selectedEntity == kNullEntity) return;


	// PSOをコマンドリストに登録する
	pso_->Register(commandList);

	// インデックスバッファを設定する
	indexResource_->Register(commandList, 0);

	// 頂点バッファを設定する
	vertexResource_->Register(commandList, 0);

	// 変換行列バッファを設定する
	auto* transformData = transformBuffer_->GetData(frameIndex);
	*transformData = cameraSystem->GetCameraMatrix() * cameraSystem->GetViewMatrix() * cameraSystem->GetProjectionMatrix();
	transformBuffer_->RegisterGraphics(commandList, 0, frameIndex);

	// カラーバッファを設定する
	auto* colorData = colorBuffer_->GetData(frameIndex);
	*colorData = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	colorBuffer_->RegisterGraphics(commandList, 1, frameIndex);

	// CubemapComponentを取得する
	auto cubemapComp = registry_->GetComponent<CubemapComponent>(selectedEntity);
	if (!cubemapComp) return;
	commandList->SetGraphicsRootDescriptorTable(2, textureStore->GetSrvHandle(cubemapComp->handle).gpuHandle);

	// プリミティブトポロジーを設定する
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// インデックス付きインスタンス描画を実行する
	commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}