#include "RenderSystem.hpp"
#include "Engine.hpp"

#include "Func/AnimationFunc/AnimationFunc.hpp"
#include "ECS/ComponentECS/DataForGPU/DataForGPU.hpp"

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
	psoEditor_ = psoEditor;

	// TransformConstantBufferComponentが削除されたときのコールバックを設定する
	registry->SetOnRemoveComponentCallback<TransformConstantBufferComponent>([this](Entity entity, const TransformConstantBufferComponent& comp)
		{
			if (renderResourceStore_)
			{
				renderResourceStore_->ReleaseConstantBuffer(comp.handle);
			}
		});

	// MaterialConstantBufferComponentが削除されたときのコールバックを設定する
	registry->SetOnRemoveComponentCallback<MaterialConstantBufferComponent>([this](Entity entity, const MaterialConstantBufferComponent& comp)
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

	// ECSレジストリを取得する
	auto registry = engine->GetRegistryECS();
	if (!registry) return kNullEntity;

	int32_t maxBufferCount = engine->GetMaxBufferCount();
	assert(maxBufferCount > 0);



	// エンティティを作成する
	Entity entity = registry->CreateEntity();

	// RenderComponentを追加する
	RenderComponent render3DComp;
	registry->AddComponent(entity, render3DComp);

	// MeshComponentを追加する
	MeshComponent meshComp;
	registry->AddComponent(entity, meshComp);

	// TransformComponentを追加する
	TransformComponent transformComp;
	registry->AddComponent(entity, transformComp);

	// MaterialComponentを追加する
	MaterialComponent materialComp;
	registry->AddComponent(entity, materialComp);

	// UVTransformComponentを追加する
	UVTransformComponent uvTransformComp;
	registry->AddComponent(entity, uvTransformComp);

	// TextureComponentを追加する
	TextureComponent textureComp(textureStore->GetDefaultTextureHandle());
	registry->AddComponent(entity, textureComp);

	// TransformDataForGPU用のConstantBufferComponentを追加する
	TransformConstantBufferComponent constantBufferComp(renderResourceStore_->CreateConstantBuffer<Detail::TransformDataForGPU>(device, maxBufferCount));
	registry->AddComponent(entity, constantBufferComp);

	// MaterialComponentを追加する
	MaterialConstantBufferComponent materialConstantBufferComp(renderResourceStore_->CreateConstantBuffer<Detail::UnLightingMaterialDataForGPU>(device, maxBufferCount));
	registry->AddComponent(entity, materialConstantBufferComp);

	return entity;
}

/// @brief モデルエンティティを作成する
/// @param modelHandle 
/// @return 
Entity Detail::RenderSystem::CreateModelEntity(ModelHandle modelHandle)
{
	auto engine = Engine::GetInstance();
	auto renderContext = engine->GetRenderContext();
	auto device = renderContext->GetCore()->GetDevice();
	auto commandList = renderContext->GetCommand()->GetCommandList();
	auto textureStore = renderContext->GetTextureStore();
	auto modelStore = renderContext->GetModelStore();
	int32_t maxBufferCount = engine->GetMaxBufferCount();

	auto registry = engine->GetRegistryECS();
	if (!registry) return kNullEntity;

	// モデルデータを取得する
	auto modelData = modelStore->GetModelData(modelHandle);
	if (!modelData || modelData->nodes.empty()) return kNullEntity;

	// 親エンティティ（ルートモデルエンティティ）の作成
	Entity rootEntity = registry->CreateEntity();
	registry->AddComponent(rootEntity, TransformComponent());
	registry->AddComponent(rootEntity, HierarchyComponent());

	ModelComponent modelComp;
	modelComp.handle = modelHandle;
	registry->AddComponent(rootEntity, modelComp);

	// アニメーションを持つかどうかを判定する
	if (modelData->hasAnimations)
	{
		auto animationStore = renderContext->GetAnimationStore();

		// アニメーションコンポーネントを追加する
		AnimationComponent animationComp;
		animationComp.handle = animationStore->Load(modelData->filePath);
		registry->AddComponent(rootEntity, animationComp); // 修正: parentEntity -> rootEntity
	}

	// スケルトンを持つかどうかを判定する
	SkeletonData* skeletonData = nullptr;
	if (modelData->hasSkeleton)
	{
		auto skeletonStore = renderContext->GetSkeletonStore();

		// スケルトンコンポーネントを追加する
		SkeletonComponent skeletonComp;
		skeletonComp.handle = skeletonStore->Load(modelData->filePath);
		skeletonData = skeletonStore->GetSkeletonData(skeletonComp.handle);
		skeletonComp.pose.Initialize(skeletonStore->GetSkeletonData(skeletonComp.handle)->skeleton);
		registry->AddComponent(rootEntity, skeletonComp); // 修正: parentEntity -> rootEntity
	}

	// ノードを再帰的に構築するためのラムダ関数
	std::function<Entity(int32_t, Entity)> CreateNodeEntity = [&](int32_t nodeIndex, Entity parentEntity) -> Entity
		{
			const auto& node = modelData->nodes[nodeIndex];
			Entity entity = registry->CreateEntity();

			// 親子関係を設定
			HierarchyComponent hierarchy;
			hierarchy.parent = parentEntity;
			registry->AddComponent(entity, hierarchy);

			// TransformComponentにノードの初期トランスフォームを設定
			TransformComponent transform;
			transform.scale = node.transform.scale;
			transform.position = node.transform.translate;
			transform.rotation = node.transform.rotate.ToEulerAngles();

			// NodeComponentを追加してノード名とインデックスを保持
			NodeComponent nodeComp;
			nodeComp.name = node.name;
			nodeComp.index = nodeIndex;
			registry->AddComponent(entity, nodeComp);

			registry->AddComponent(entity, transform);

			// ノードがメッシュを持っている場合、描画コンポーネントを追加
			if (node.meshIndex.has_value())
			{
				int32_t meshIdx = node.meshIndex.value();

				registry->AddComponent(entity, RenderComponent());

				MeshDrawComponent meshDrawComp(meshIdx);
				meshDrawComp.handle = modelHandle;
				meshDrawComp.rootMeshEntity = rootEntity;
				registry->AddComponent(entity, meshDrawComp);

				registry->AddComponent(entity, MaterialComponent());
				registry->AddComponent(entity, UVTransformComponent());
				registry->AddComponent(entity, TextureComponent(modelData->meshes[meshIdx].material.handle));

				// GPUデータ用のコンスタントバッファ
				registry->AddComponent(entity, TransformConstantBufferComponent(
					renderResourceStore_->CreateConstantBuffer<Detail::TransformDataForGPU>(device, maxBufferCount)));
				registry->AddComponent(entity, MaterialConstantBufferComponent(
					renderResourceStore_->CreateConstantBuffer<Detail::UnLightingMaterialDataForGPU>(device, maxBufferCount)));

				// スケルトンデータが存在する場合はスキニングインスタンスとスキンクラスターを追加する
				if (skeletonData)
				{
					auto skinClusterStore = renderContext->GetSkinClusterStore();
					auto skinInstanceStore = renderContext->GetSkinningInstanceStore();

					// スキニングインスタンスを追加する
					SkinningInstanceComponent skinningInstanceComp;
					skinningInstanceComp.handle = skinInstanceStore->Load(renderContext->GetHeap(), device, skeletonData->skeleton, modelData->meshes[meshIdx], skeletonData->boneData.meshes[meshIdx]);
					registry->AddComponent(entity, skinningInstanceComp);

					// スキンクラスターを追加する
					SkinClusterComponent skinClusterComp;
					skinClusterComp.handle = skinClusterStore->Load(renderContext->GetHeap(), device, commandList, modelData->meshes[meshIdx], skeletonData->skeleton);
					registry->AddComponent(entity, skinClusterComp);
				}
			}

			// 子ノードを再帰的に作成して HierarchyComponent に登録
			std::vector<Entity> childEntities;
			for (int32_t childIndex : node.children)
			{
				childEntities.push_back(CreateNodeEntity(childIndex, entity));
			}

			// HierarchyComponent に子エンティティを設定
			if (auto currentHierarchy = registry->GetComponent<HierarchyComponent>(entity))
			{
				currentHierarchy->children = std::move(childEntities);
			}

			return entity;
		};

	// ルートノードの子エンティティを作成する
	std::vector<Entity> rootChildren;
	if (!modelData->nodes.empty())
	{
		rootChildren.push_back(CreateNodeEntity(0, rootEntity));
	}

	// 親エンティティのHierarchyComponentに子エンティティを設定する
	if (auto safeParentHierarchy = registry->GetComponent<HierarchyComponent>(rootEntity))
	{
		safeParentHierarchy->children = std::move(rootChildren);
	}

	return rootEntity;
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

	auto modelStore = engine->GetRenderContext()->GetModelStore();
	if (!modelStore) return;

	auto registry = engine->GetRegistryECS();
	if (!registry) return;

	// RenderComponentの配列を取得する
	auto* passArray = registry->GetComponentArray<RenderComponent>();
	const auto& entities = passArray->GetEntities();

	// priority に基づいてソートするためのペアのベクターを作成
	std::vector<Entity> sortedPasses;
	for (Entity entity : entities)
	{
		auto pass = registry->GetComponent<RenderComponent>(entity);
		if (pass && pass->isEnabled)
		{
			sortedPasses.push_back(entity);
		}
	}

	// ソートされたレンダーパスが空の場合は何もしない
	if (sortedPasses.empty()) return;

	// ソートされたレンダーパスを実行する
	for (const auto& entity : sortedPasses)
	{
		// メッシュインデックスを設定
		auto meshDrawComp = registry->GetComponent<MeshDrawComponent>(entity);
		if (meshDrawComp == nullptr) continue;

		// モデルデータを取得する
		auto modelData = modelStore->GetModelData(meshDrawComp->handle);
		if (!modelData || meshDrawComp->meshIndex < 0 || meshDrawComp->meshIndex >= modelData->meshes.size()) continue;

		// 深度のみのPSOを登録する
		depthOnlyPso_->Register(commandList);

		// ループを削除
		auto transformCB = registry->GetComponent<TransformConstantBufferComponent>(entity);

		// コンポーネントが存在しない場合はスキップする
		if (!registry->GetComponent<TransformComponent>(entity) || !transformCB)continue;

		// meshIndex を使用
		modelStore->Register(commandList, meshDrawComp->handle, meshDrawComp->meshIndex);

		{
			// データをGPU用に渡す
			TransformDataForGPU data;
			data.world = CalculateGlobalWorldMatrix(entity, meshDrawComp->rootMeshEntity);
			data.worldViewProjection = data.world * cameraSystem->GetViewMatrix() * cameraSystem->GetProjectionMatrix();
			data.worldInverseTranspose = data.world.Inverse().Transpose();

			// Transform用のリソースをバインド
			renderResourceStore_->UpdateConstantBuffer(transformCB->handle, data, frameIndex);
			auto cbResource = renderResourceStore_->GetConstantBuffer(transformCB->handle);
			if (cbResource)cbResource->RegisterGraphics(commandList, 0, frameIndex);
		}

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawIndexedInstanced(modelData->meshes[meshDrawComp->meshIndex].indexBufferResource->GetNumIndex(), 1, 0, 0, 0);
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

	auto modelStore = engine->GetRenderContext()->GetModelStore();
	if (!modelStore) return;

	// ECSレジストリを取得する
	auto registry = engine->GetRegistryECS();
	if (!registry) return;

	// RenderComponentの配列を取得する
	auto* passArray = registry->GetComponentArray<RenderComponent>();
	const auto& entities = passArray->GetEntities();

	// 今のフレームインデックスを取得する
	int32_t frameIndex = engine->GetFrameIndex();

	// priority に基づいてソートするためのペアのベクターを作成
	std::vector<std::pair<Entity, int>> sortedPasses;
	for (Entity entity : entities)
	{
		auto pass = registry->GetComponent<RenderComponent>(entity);
		if (pass && pass->isEnabled && pass->priority == priority)
		{
			sortedPasses.push_back({ entity, pass->priority });
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
		auto meshDrawComp = registry->GetComponent<MeshDrawComponent>(entity);
		if (!meshDrawComp) continue;

		// モデルデータを取得する
		auto modelData = modelStore->GetModelData(meshDrawComp->handle);
		if (!modelData || meshDrawComp->meshIndex < 0 || meshDrawComp->meshIndex >= modelData->meshes.size()) continue;

		// ループを削除し、エンティティの components を直接取得
		auto material = registry->GetComponent<MaterialComponent>(entity);
		auto uvTransform = registry->GetComponent<UVTransformComponent>(entity);
		auto texture = registry->GetComponent<TextureComponent>(entity);
		auto transformCB = registry->GetComponent<TransformConstantBufferComponent>(entity);
		auto materialCB = registry->GetComponent<MaterialConstantBufferComponent>(entity);
		if (!material || !uvTransform || !texture || !transformCB || !materialCB) continue;

		// PSOを取得する
		auto psoItem = psoEditor_->GetPSOItem(material->psoName);
		if (!psoItem || !psoItem->pso) continue;

		// PSOとルートシグネチャを設定する
		commandList->SetPipelineState(psoItem->pso.Get());
		commandList->SetGraphicsRootSignature(psoItem->rootSig.Get());

		//meshIndex を使用して登録
		modelStore->Register(commandList, meshDrawComp->handle, meshDrawComp->meshIndex);

		// PSOの要求に従って動的にパラメータをバインドする
		for (UINT i = 0;i < psoItem->desc.rootParameters.size(); ++i)
		{
			const auto& param = psoItem->desc.rootParameters[i];

			// RenderBindingに基づいて適切なリソースをバインドする
			switch (param.binding)
			{
			case RenderBinding::Transform:
			{
				// Transform用のデータをGPU用に渡す
				TransformDataForGPU data;
				data.world = CalculateGlobalWorldMatrix(entity, meshDrawComp->rootMeshEntity);
				data.worldViewProjection = data.world * cameraSystem->GetViewMatrix() * cameraSystem->GetProjectionMatrix();
				data.worldInverseTranspose = data.world.Inverse().Transpose();

				// Transform用のリソースをバインド
				renderResourceStore_->UpdateConstantBuffer(transformCB->handle, data, frameIndex);
				auto cbResource = renderResourceStore_->GetConstantBuffer(transformCB->handle);
				if (cbResource)cbResource->RegisterGraphics(commandList, i, frameIndex);
				break;
			}
			case RenderBinding::Material:
			{
				UnLightingMaterialDataForGPU data;
				data.color = material->color; // マテリアルの色を使用
				data.uvTransform =
					Make3DScaleMatrix4x4(Vector3(uvTransform->scale.x, uvTransform->scale.y, 1.0f)) *
					Make3DRotateZMatrix4x4(uvTransform->radian) *
					Make3DTranslateMatrix4x4(Vector3(uvTransform->offset.x, uvTransform->offset.y, 0.0f));

				// Material用のリソースをバインド
				renderResourceStore_->UpdateConstantBuffer(materialCB->handle, data, frameIndex);
				auto cbResource = renderResourceStore_->GetConstantBuffer(materialCB->handle);
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
			case RenderBinding::NumLights:
			{
				// NumLights用のリソースをバインド
				auto cbResource = lightSystem->GetNumLightDataBuffer();
				if (cbResource)cbResource->RegisterGraphics(commandList, i, frameIndex);
				break;
			}
			case RenderBinding::DirectionalLight:
			{
				// DirectionalLight用のリソースをバインド
				auto cbResource = lightSystem->GetDirectionalLightDataBuffer();
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
				commandList->SetGraphicsRootDescriptorTable(i, textureStore->GetSrvHandle(texture->handle).gpuHandle);
				break;
			}

			}
		}

		// プリミティブトポロジーを設定する
		if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		else if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		else if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

		// meshIndex を使用してドローコール
		commandList->DrawIndexedInstanced(modelData->meshes[meshDrawComp->meshIndex].indexBufferResource->GetNumIndex(), 1, 0, 0, 0);
	}
}

/// @brief グローバルなワールド行列を計算する
/// @param entity 
/// @param isAnimation 
/// @param meshIndex 
/// @return 
Matrix4x4 Detail::RenderSystem::CalculateGlobalWorldMatrix(Entity entity, Entity rootEntity)
{
	auto registry = Engine::GetInstance()->GetRegistryECS();
	if (!registry) return Matrix4x4();

	auto transform = registry->GetComponent<TransformComponent>(entity);
	if (!transform) return Matrix4x4(); // TransformComponentが存在しない場合は単位行列を返す

	// 自身のローカル行列を計算
	Quaternion rotation =
		ToQuaternion(transform->rotation.z, Vector3(0.0f, 0.0f, 1.0f)) *
		ToQuaternion(transform->rotation.y, Vector3(0.0f, 1.0f, 0.0f)) *
		ToQuaternion(transform->rotation.x, Vector3(1.0f, 0.0f, 0.0f));

	// ローカル行列を計算
	Matrix4x4 localWorld = Make3DAffineMatrix4x4(transform->scale, rotation, transform->position);

	

	// アニメーションが存在する場合はアニメーションの変換を適用
	auto animationComp = registry->GetComponent<AnimationComponent>(rootEntity);
	auto nodeComp = registry->GetComponent<NodeComponent>(entity);

	// アニメーションが存在する場合はアニメーションの変換を適用
	if (animationComp)
	{
		auto animationStore = Engine::GetInstance()->GetRenderContext()->GetAnimationStore();
		auto animationData = animationStore->GetAnimationData(animationComp->handle);

		if (animationData)
		{
			int32_t animNodeIndex = -1;

			// NodeComponentが存在する場合は、ノード名からアニメーションノードのインデックスを取得
			if (nodeComp)
			{
				auto it = animationData->indices.find(nodeComp->name);
				if (it != animationData->indices.end())
				{
					animNodeIndex = it->second;
				}
			}

			// アニメーションノードが見つかった場合は、アニメーションの変換を適用
			if (animNodeIndex >= 0 && animNodeIndex < static_cast<int32_t>(animationData->nodes.size()))
			{
				const auto& animNode = animationData->nodes[animNodeIndex];

				Vector3 animationTranslate = animNode.translate.empty() ? Vector3() : CalculateValue(animNode.translate, animationComp->currentTime);
				Quaternion animationRotate = animNode.rotate.empty() ? Quaternion() : CalculateValue(animNode.rotate, animationComp->currentTime);
				Vector3 animationScale = animNode.scale.empty() ? Vector3(1.0f, 1.0f, 1.0f) : CalculateValue(animNode.scale, animationComp->currentTime);

				// アニメーションの変換をローカル行列に適用
				Matrix4x4 animationMatrix = Make3DAffineMatrix4x4(animationScale, animationRotate, animationTranslate);
				localWorld = animationMatrix * localWorld;
			}
		}
	}


	// 親子関係コンポーネントがあり、かつ親が設定されている場合は親の行列を掛ける
	if (registry->HasComponent<HierarchyComponent>(entity))
	{
		auto hierarchy = registry->GetComponent<HierarchyComponent>(entity);
		if (hierarchy && hierarchy->parent != kNullEntity)
		{
			Matrix4x4 parentWorld = CalculateGlobalWorldMatrix(hierarchy->parent, rootEntity);

			return localWorld * parentWorld;
		}
	}

	return localWorld;
}