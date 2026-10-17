#include "RenderSystem.h"
#include "Engine.h"

#include "ECS/ComponentECS/DataForGPU/DataForGPU.h"

/// @brief 初期化
void Detail::RenderSystem::Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler)
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

	// PSOエディタを取得する
	auto psoEditor = engine->GetPSOEditor();
	assert(psoEditor);

	// メンバ変数に代入する
	registry_ = registry;
	psoEditor_ = psoEditor;

	// TransformConstantBufferComponentが削除されたときのコールバックを設定する
	registry_->SetOnRemoveComponentCallback<TransformConstantBufferComponent>([this](Entity entity, const TransformConstantBufferComponent& comp)
		{
			if (renderResourceStore_)
			{
				renderResourceStore_->ReleaseConstantBuffer(comp.handle);
			}
		});

	// MaterialConstantBufferComponentが削除されたときのコールバックを設定する
	registry_->SetOnRemoveComponentCallback<MaterialConstantBufferComponent>([this](Entity entity, const MaterialConstantBufferComponent& comp)
		{
			if (renderResourceStore_)
			{
				renderResourceStore_->ReleaseConstantBuffer(comp.handle);
			}
		});

	/// レンダーリソースストアを生成する
	renderResourceStore_ = std::make_unique<RenderResourceStore>();

	// 深度のみのPSOを生成する
	depthOnlyPso_ = std::make_unique<DepthOnlyPSO>(device, shaderCompiler);
}

/// @brief エンティティを作成する
/// @return 
Entity Detail::RenderSystem::CreateRender3DEntity()
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

	// RenderComponentを追加する
	RenderComponent render3DComp;
	registry_->AddComponent(entity, render3DComp);

	// MeshComponentを追加する
	MeshComponent meshComp;
	registry_->AddComponent(entity, meshComp);

	// TransformComponentを追加する
	TransformComponent transformComp;
	registry_->AddComponent(entity, transformComp);

	// MaterialComponentを追加する
	MaterialComponent materialComp;
	registry_->AddComponent(entity, materialComp);

	// UVTransformComponentを追加する
	UVTransformComponent uvTransformComp;
	registry_->AddComponent(entity, uvTransformComp);

	// TextureComponentを追加する
	TextureComponent textureComp(textureStore->GetDefaultTextureHandle());
	registry_->AddComponent(entity, textureComp);

	// TransformDataForGPU用のConstantBufferComponentを追加する
	TransformConstantBufferComponent constantBufferComp(renderResourceStore_->CreateConstantBuffer<Detail::TransformDataForGPU>(device, maxBufferCount));
	registry_->AddComponent(entity, constantBufferComp);

	// MaterialComponentを追加する
	MaterialConstantBufferComponent materialConstantBufferComp(renderResourceStore_->CreateConstantBuffer<Detail::UnLightingMaterialDataForGPU>(device, maxBufferCount));
	registry_->AddComponent(entity, materialConstantBufferComp);

	return entity;
}

/// @brief 描画前の深度パスを実行する
/// @param commandList 
/// @param priority 
void Detail::RenderSystem::ExecuteDepthPrePass(ID3D12GraphicsCommandList* commandList, int32_t frameIndex)
{
	auto engine = Engine::GetInstance();
	if (!engine) return;

	auto textureStore = engine->GetRenderContext()->GetTextureStore();
	if (!textureStore) return;

	auto proceduralMeshStore = engine->GetRenderContext()->GetProceduralMeshStore();
	if (!proceduralMeshStore) return;

	auto cameraSystem = engine->GetRenderContext()->GetCameraSystem();
	if (!cameraSystem) return;

	// RenderComponentの配列を取得する
	auto* passArray = registry_->GetComponentArray<RenderComponent>();
	const auto& entities = passArray->GetEntities();

	// priority に基づいてソートするためのペアのベクターを作成
	std::vector<Entity> sortedPasses;
	for (Entity entity : entities)
	{
		auto& pass = registry_->GetComponent<RenderComponent>(entity);
		if (pass.isEnabled)
		{
			sortedPasses.push_back(entity);
		}
	}

	// ソートされたレンダーパスが空の場合は何もしない
	if (sortedPasses.empty()) return;

	// ソートされたレンダーパスを実行する
	for (const auto& entity : sortedPasses)
	{
		// 必要なコンポーネントを取得
		auto& transform = registry_->GetComponent<TransformComponent>(entity);
		auto& transformCB = registry_->GetComponent<TransformConstantBufferComponent>(entity);
		auto& mesh = registry_->GetComponent<MeshComponent>(entity);


		// 深度のみのPSOを登録する
		depthOnlyPso_->Register(commandList);

		// ProceduralMeshStoreからメッシュを登録する
		proceduralMeshStore->Register(mesh.meshType, commandList);

		{
			// 回転をクォータニオンに変換する
			Quaternion rotation =
				ToQuaternion(transform.rotation.z, Vector3(0.0f, 0.0f, 1.0f)) *
				ToQuaternion(transform.rotation.y, Vector3(0.0f, 1.0f, 0.0f)) *
				ToQuaternion(transform.rotation.x, Vector3(1.0f, 0.0f, 0.0f));

			// データをGPU用に渡す
			TransformDataForGPU data;
			data.world = Make3DAffineMatrix4x4(transform.scale, rotation, transform.position);
			data.worldViewProjection = data.world * cameraSystem->GetViewMatrix() * cameraSystem->GetProjectionMatrix();
			data.worldInverseTranspose = data.world.Inverse().Transpose();

			// Transform用のリソースをバインド
			renderResourceStore_->UpdateConstantBuffer(transformCB.handle, data, frameIndex);
			auto cbResource = renderResourceStore_->GetConstantBuffer(transformCB.handle);
			if (cbResource)cbResource->RegisterGraphics(commandList, 0, frameIndex);
		}


		// プリミティブトポロジーを設定
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// ドローコール
		commandList->DrawIndexedInstanced(proceduralMeshStore->GetNumIndex(mesh.meshType), 1, 0, 0, 0);
	}
}

/// @brief すべての3D描画を実行する
/// @param commandList 
/// @param priority 
void Detail::RenderSystem::ExecuteAll(ID3D12GraphicsCommandList* commandList, int priority)
{
	// priority が負の値の場合は何もしない
	if (priority < 0)return;

	auto engine = Engine::GetInstance();
	if (!engine) return;

	auto textureStore = engine->GetRenderContext()->GetTextureStore();
	if (!textureStore) return;

	auto proceduralMeshStore = engine->GetRenderContext()->GetProceduralMeshStore();
	if (!proceduralMeshStore) return;

	auto cameraSystem = engine->GetRenderContext()->GetCameraSystem();
	if (!cameraSystem) return;

	auto lightSystem = engine->GetRenderContext()->GetLightSystem();
	if (!lightSystem) return;

	// RenderComponentの配列を取得する
	auto* passArray = registry_->GetComponentArray<RenderComponent>();
	const auto& entities = passArray->GetEntities();

	// 今のフレームインデックスを取得する
	int32_t frameIndex = engine->GetFrameIndex();

	// priority に基づいてソートするためのペアのベクターを作成
	std::vector<std::pair<Entity, int>> sortedPasses;
	for (Entity entity : entities)
	{
		auto& pass = registry_->GetComponent<RenderComponent>(entity);
		if (pass.isEnabled && pass.priority == priority)
		{
			sortedPasses.push_back({ entity, pass.priority });
		}
	}

	// ソートされたレンダーパスが空の場合は何もしない
	if (sortedPasses.empty()) return;

	// priority に基づいて昇順でソート（数値が小さいほど先に描画）
	std::sort(sortedPasses.begin(), sortedPasses.end(), [](const auto& a, const auto& b) {return a.second < b.second;});

	// ソートされたレンダーパスを実行する
	for (const auto& [entity, priority] : sortedPasses)
	{
		// 必要なコンポーネントを取得
		auto& transform = registry_->GetComponent<TransformComponent>(entity);
		auto& material = registry_->GetComponent<MaterialComponent>(entity);
		auto& uvTransform = registry_->GetComponent<UVTransformComponent>(entity);
		auto& mesh = registry_->GetComponent<MeshComponent>(entity);
		auto& texture = registry_->GetComponent<TextureComponent>(entity);

		auto& transformCB = registry_->GetComponent<TransformConstantBufferComponent>(entity);
		auto& materialCB = registry_->GetComponent<MaterialConstantBufferComponent>(entity);

		// PSOエディタ（または管理クラス）からPSOとルートシグネチャを取得
		auto psoItem = psoEditor_->GetPSOItem(material.psoName);
		if (!psoItem || !psoItem->pso) continue;


		// PSOとルートシグネチャをコマンドリストに設定
		commandList->SetPipelineState(psoItem->pso.Get());
		commandList->SetGraphicsRootSignature(psoItem->rootSig.Get());


		// ProceduralMeshStoreからメッシュを登録する
		proceduralMeshStore->Register(mesh.meshType, commandList);

		// PSOの要求に従って動的にパラメータをバインドする
		for (UINT i = 0;i < psoItem->desc.rootParameters.size(); ++i)
		{
			const auto& param = psoItem->desc.rootParameters[i];

			// RenderBindingに基づいて適切なリソースをバインドする
			switch (param.binding)
			{
			case RenderBinding::Transform:
			{
				// 回転をクォータニオンに変換する
				Quaternion rotation = 
					ToQuaternion(transform.rotation.z , Vector3(0.0f, 0.0f, 1.0f)) * 
					ToQuaternion(transform.rotation.y , Vector3(0.0f, 1.0f, 0.0f)) * 
					ToQuaternion(transform.rotation.x , Vector3(1.0f, 0.0f, 0.0f));


				TransformDataForGPU data;
				data.world = Make3DAffineMatrix4x4(transform.scale, rotation, transform.position);
				data.worldViewProjection = data.world * cameraSystem->GetViewMatrix() * cameraSystem->GetProjectionMatrix();
				data.worldInverseTranspose = data.world.Inverse().Transpose();

				// Transform用のリソースをバインド
				renderResourceStore_->UpdateConstantBuffer(transformCB.handle, data, frameIndex);
				auto cbResource = renderResourceStore_->GetConstantBuffer(transformCB.handle);
				if (cbResource)cbResource->RegisterGraphics(commandList, i, frameIndex);
				break;
			}
			case RenderBinding::Material:
			{
				UnLightingMaterialDataForGPU data;
				data.color = material.color; // マテリアルの色を使用
				data.uvTransform = 
					Make3DScaleMatrix4x4(Vector3(uvTransform.scale.x, uvTransform.scale.y, 1.0f)) *
					Make3DRotateZMatrix4x4(uvTransform.radian) * 
					Make3DTranslateMatrix4x4(Vector3(uvTransform.offset.x, uvTransform.offset.y, 0.0f));

				// Material用のリソースをバインド
				renderResourceStore_->UpdateConstantBuffer(materialCB.handle, data, frameIndex);
				auto cbResource = renderResourceStore_->GetConstantBuffer(materialCB.handle);
				if (cbResource)cbResource->RegisterGraphics(commandList, i, frameIndex);
				break;
			}
			case RenderBinding::Camera:
			{
				// Camera用のリソースをバインド
				auto cbResource = lightSystem->GetCameraDataBuffer();
				if (cbResource)cbResource->RegisterGraphics(commandList, i, frameIndex);
				break;
			}
			case RenderBinding::PointLight:
			{
				// PointLight用のリソースをバインド
				auto cbResource = lightSystem->GetPointLightDataBuffer();
				if (cbResource)cbResource->RegisterGraphics(commandList, i, frameIndex);
				break;
			}
			case RenderBinding::SpotLight:
			{
				// SpotLight用のリソースをバインド
				auto cbResource = lightSystem->GetSpotLightDataBuffer();
				if (cbResource)cbResource->RegisterGraphics(commandList, i, frameIndex);
				break;
			}
			case RenderBinding::LightCullingTitle:
			{
				// LightCullingTitle用のリソースをバインド
				auto cbResource = lightSystem->GetTileLightIndexBuffer();
				if (cbResource)cbResource->RegisterGraphicsSRV(commandList, i, frameIndex);
				break;
			}
			case RenderBinding::Texture:
			{
				// Texture用のリソースをバインド
				commandList->SetGraphicsRootDescriptorTable(i, textureStore->GetSrvHandle(texture.handle).gpuHandle);
				break;
			}

			}
		}

		// トポロジを設定する
		if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		else if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		else if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

		// ドローコール
		commandList->DrawIndexedInstanced(proceduralMeshStore->GetNumIndex(mesh.meshType), 1, 0, 0, 0);
	}
}