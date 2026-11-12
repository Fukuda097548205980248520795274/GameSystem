#include "RenderSystem.hpp"
#include "Engine.hpp"

#include "Func/AnimationFunc/AnimationFunc.hpp"
#include "ECS/ComponentECS/DataForGPU/DataForGPU.hpp"

#include "Func/SkeletonFunc/SkeletonFunc.hpp"

/// @brief 初期化
void Detail::RenderSystem::Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler)
{
	// nullptrチェック
	assert(device);
	assert(shaderCompiler);


	// Engineのインスタンスを取得する
	auto  engine = Engine::GetInstance();
	assert(engine);

	// PSOエディタを取得する
	auto psoEditor = engine->GetPSOEditor();
	assert(psoEditor);

	// メンバ変数に代入する
	psoEditor_ = psoEditor;

	// 深度のみのPSOを生成する
	depthOnlyPso_ = std::make_unique<DepthOnlyPSO>(device, shaderCompiler);

	// スキニング用PSOを生成する
	skinningPso_ = std::make_unique<SkinningPSO>(device, shaderCompiler);
}

/// @brief 更新処理
/// @param frameIndex 
void Detail::RenderSystem::Update(ID3D12GraphicsCommandList* commandList, int32_t frameIndex)
{
	// トランスフォームの更新
	UpdateTransform(frameIndex);

	// スキニング処理を行う
	SkinningExecuteAll(commandList, frameIndex);
}

/// @brief エンティティを作成する
/// @return 
Entity Detail::RenderSystem::CreateRender3DEntity()
{
	auto engine = Engine::GetInstance();
	if(!engine) return kNullEntity;

	auto renderContext = engine->GetRenderContext();
	if(!renderContext) return kNullEntity;

    auto resourceStore = renderContext->GetResourceStore();
    if(!resourceStore) return kNullEntity;

    auto uploadBufferAllocator = resourceStore->GetUploadBufferAllocator();
    if(!uploadBufferAllocator) return kNullEntity;

	auto device = renderContext->GetCore()->GetDevice();
	if(!device) return kNullEntity;

	auto textureStore = renderContext->GetTextureStore();
	if(!textureStore) return kNullEntity;

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
	TransformConstantBufferComponent constantBufferComp(resourceStore->Create<ConstantBufferResource<TransformDataForGPU>>(uploadBufferAllocator, maxBufferCount));
	registry->AddComponent(entity, constantBufferComp);

	// MaterialComponentを追加する
	MaterialConstantBufferComponent materialConstantBufferComp(resourceStore->Create<ConstantBufferResource<UnLightingMaterialDataForGPU>>(uploadBufferAllocator, maxBufferCount));
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
    auto resourceStore = renderContext->GetResourceStore();
    auto uploadBufferAllocator = resourceStore->GetUploadBufferAllocator();
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
    registry->AddComponent(rootEntity, JointHierarchyComponent()); // ルートにもJointHierarchyを追加

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
        registry->AddComponent(rootEntity, animationComp);
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
        registry->AddComponent(rootEntity, skeletonComp);
    }

    // ノードを再帰的に構築するためのラムダ関数
    std::function<Entity(int32_t, Entity)> CreateNodeEntity = [&](int32_t nodeIndex, Entity parentEntity) -> Entity
        {
            const auto& node = modelData->nodes[nodeIndex];
            Entity entity = registry->CreateEntity();

            // スケルトンのジョイントかどうかを判定する
            bool isJoint = false;
            if (skeletonData)
            {
                if (skeletonData->skeleton.jointMap.find(node.name) != skeletonData->skeleton.jointMap.end())
                {
                    isJoint = true;
                }
            }

            // 判定結果をもとに親子関係コンポーネントを設定
            if (isJoint)
            {
                JointHierarchyComponent hierarchy;
                hierarchy.parent = parentEntity;
                registry->AddComponent(entity, hierarchy);
            }
            else
            {
                HierarchyComponent hierarchy;
                hierarchy.parent = parentEntity;
                registry->AddComponent(entity, hierarchy);
            }

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

            // ノードがメッシュを持っている、かつ「ジョイントではない」場合のみ描画コンポーネントを追加
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
                    resourceStore->Create<ConstantBufferResource<TransformDataForGPU>>(uploadBufferAllocator, maxBufferCount)));
                registry->AddComponent(entity, MaterialConstantBufferComponent(
                    resourceStore->Create<ConstantBufferResource<UnLightingMaterialDataForGPU>>(uploadBufferAllocator, maxBufferCount)));

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

            // 子ノードを再帰的に作成
            std::vector<Entity> meshChildEntities;
            std::vector<Entity> jointChildEntities;

            for (int32_t childIndex : node.children)
            {
                const auto& childNode = modelData->nodes[childIndex];
                bool childIsJoint = false;
                if (skeletonData && skeletonData->skeleton.jointMap.find(childNode.name) != skeletonData->skeleton.jointMap.end())
                {
                    childIsJoint = true;
                }

                // 子ノードがジョイントであり、親(自分)がジョイントでない場合は、ルート配下のジョイントツリーとして接続
                if (childIsJoint && !isJoint)
                {
                    Entity jointChild = CreateNodeEntity(childIndex, rootEntity);
                    if (auto rootJointHierarchy = registry->GetComponent<JointHierarchyComponent>(rootEntity))
                    {
                        rootJointHierarchy->children.push_back(jointChild);
                    }
                }
                else
                {
                    Entity childEntity = CreateNodeEntity(childIndex, entity);
                    if (childIsJoint)
                    {
                        jointChildEntities.push_back(childEntity);
                    }
                    else
                    {
                        meshChildEntities.push_back(childEntity);
                    }
                }
            }

            // 追加したコンポーネントに合わせて子エンティティを登録
            if (isJoint)
            {
                if (auto currentHierarchy = registry->GetComponent<JointHierarchyComponent>(entity))
                {
                    currentHierarchy->children = std::move(jointChildEntities);
                }
            }
            else
            {
                if (auto currentHierarchy = registry->GetComponent<HierarchyComponent>(entity))
                {
                    currentHierarchy->children = std::move(meshChildEntities);
                }
            }

            return entity;
        };

    // ルートノードの子エンティティを作成する
    if (!modelData->nodes.empty())
    {
        const auto& rootNode = modelData->nodes[0];
        bool isRootJoint = false;
        if (skeletonData && skeletonData->skeleton.jointMap.find(rootNode.name) != skeletonData->skeleton.jointMap.end())
        {
            isRootJoint = true;
        }

        if (isRootJoint)
        {
            Entity jointRoot = CreateNodeEntity(0, rootEntity);
            if (auto rootJointHierarchy = registry->GetComponent<JointHierarchyComponent>(rootEntity))
            {
                rootJointHierarchy->children.push_back(jointRoot);
            }
        }
        else
        {
            Entity meshRoot = CreateNodeEntity(0, rootEntity);
            if (auto rootMeshHierarchy = registry->GetComponent<HierarchyComponent>(rootEntity))
            {
                rootMeshHierarchy->children.push_back(meshRoot);
            }
        }
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

    auto renderContext = engine->GetRenderContext();
    if (!renderContext) return;

    auto resourceStore = renderContext->GetResourceStore();
    if (!resourceStore) return;

	auto textureStore = renderContext->GetTextureStore();
	if (!textureStore) return;

	auto proceduralMeshStore = renderContext->GetProceduralMeshStore();
	if (!proceduralMeshStore) return;

	auto cameraSystem = renderContext->GetCameraSystem();
	if (!cameraSystem) return;

	auto modelStore = renderContext->GetModelStore();
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

		// Transform用のリソースをバインド
		auto cbResource = resourceStore->Get<ConstantBufferResource<TransformDataForGPU>>(transformCB->handle);
		if (cbResource)cbResource->RegisterGraphics(commandList, 0, frameIndex);

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

    auto renderContext = engine->GetRenderContext();
    if (!renderContext) return;

    auto resourceStore = renderContext->GetResourceStore();
    if (!resourceStore) return;

	auto textureStore = renderContext->GetTextureStore();
	if (!textureStore) return;

	auto proceduralMeshStore = renderContext->GetProceduralMeshStore();
	if (!proceduralMeshStore) return;

	auto cameraSystem = renderContext->GetCameraSystem();
	if (!cameraSystem) return;

	auto lightSystem = renderContext->GetLightSystem();
	if (!lightSystem) return;

	auto modelStore = renderContext->GetModelStore();
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


		// スキニングコンポーネントを取得する
		auto skeletonComp = registry->GetComponent<SkeletonComponent>(meshDrawComp->rootMeshEntity);
		auto skinningComp = registry->GetComponent<SkinningInstanceComponent>(entity);
		auto skinClusterComp = registry->GetComponent<SkinClusterComponent>(entity);

		// スキニングコンポーネントとスキンクラスターが存在する場合
		if (skeletonComp && skinningComp && skinClusterComp)
		{
			// スキニングインスタンスを取得する
			auto skinClusterStore = engine->GetRenderContext()->GetSkinClusterStore();
			if (!skinClusterStore) continue;

			// スキンクラスターを取得する
			auto skinClusterData = skinClusterStore->GetSkinCluster(skinClusterComp->handle);
			if (!skinClusterData) continue;

			// スキンクラスターの頂点データをGPUに登録する
			skinClusterData->RegisterVertex(commandList, frameIndex);
			modelStore->RegisterIndexBuffer(commandList, meshDrawComp->handle, meshDrawComp->meshIndex);
		}
		else
		{
			// スキニングコンポーネントが存在しない場合は、通常の描画処理を行う
			modelStore->Register(commandList, meshDrawComp->handle, meshDrawComp->meshIndex);
		}

		// PSOの要求に従って動的にパラメータをバインドする
		for (UINT i = 0;i < psoItem->desc.rootParameters.size(); ++i)
		{
			const auto& param = psoItem->desc.rootParameters[i];

			// RenderBindingに基づいて適切なリソースをバインドする
			switch (param.binding)
			{
			case RenderBinding::Transform:
			{
				// Transform用のリソースをバインド
                auto cbResource = resourceStore->Get<ConstantBufferResource<TransformDataForGPU>>(transformCB->handle);
                if (cbResource) cbResource->RegisterGraphics(commandList, i, frameIndex);

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
                if(auto cb = resourceStore->Get<ConstantBufferResource<UnLightingMaterialDataForGPU>>(materialCB->handle) )
				{
					if(auto mappedData = cb->GetData(frameIndex))
                        *mappedData = data;

                    cb->RegisterGraphics(commandList, i, frameIndex);
				}

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

/// @brief スキニング処理を行う
/// @param commandList 
/// @param priority 
/// @param frameIndex 
void Detail::RenderSystem::SkinningExecuteAll(ID3D12GraphicsCommandList* commandList, int32_t frameIndex)
{
	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();
	if (!engine) return;

	// モデルストアを取得する
	auto modelStore = engine->GetRenderContext()->GetModelStore();
	if (!modelStore) return;

	// アニメーションストアを取得する
	auto animationStore = engine->GetRenderContext()->GetAnimationStore();
	if (!animationStore) return;

	// スケルトンストアを取得する
	auto skeletonStore = engine->GetRenderContext()->GetSkeletonStore();
	if (!skeletonStore) return;

	// スキニングインスタンスのストアを取得する
	auto skinningInstanceStore = engine->GetRenderContext()->GetSkinningInstanceStore();
	if (!skinningInstanceStore) return;

	// スキンクラスターのストアを取得する
	auto skinClusterStore = engine->GetRenderContext()->GetSkinClusterStore();
	if (!skinClusterStore) return;

	// ECSレジストリを取得する
	auto registry = engine->GetRegistryECS();
	if (!registry) return;



	// RenderComponentの配列を取得する
	auto* passArray = registry->GetComponentArray<RenderComponent>();
	const auto& entities = passArray->GetEntities();

	// priority に基づいてソートするためのペアのベクターを作成
	std::vector<Entity> sortedPasses;
	for (Entity entity : entities)
	{
		// RenderComponentを取得する
		auto pass = registry->GetComponent<RenderComponent>(entity);
		if (!(pass && pass->isEnabled))continue;

		// MeshDrawComponentを取得する
		auto meshDrawComp = registry->GetComponent<MeshDrawComponent>(entity);
		if (!meshDrawComp)continue;

		// SkeletonComponentが存在しない場合はスキップする
		if (!registry->GetComponent<SkeletonComponent>(meshDrawComp->rootMeshEntity))continue;

		// メッシュ描画コンポーネントが存在しない場合はスキップする
		if (!(registry->GetComponent<SkinClusterComponent>(entity) && registry->GetComponent<SkinningInstanceComponent>(entity)))continue;

		// エンティティを追加する
		sortedPasses.push_back(entity);
	}

	// ソートされたレンダーパスが空の場合は何もしない
	if (sortedPasses.empty())return;


	// スキニング用PSOを登録する
	skinningPso_->Register(commandList);

	// ソートされたレンダーパスを実行する
	for (const auto& entity : sortedPasses)
	{
		// メッシュ描画コンポーネントを取得する
		auto meshDrawComp = registry->GetComponent<MeshDrawComponent>(entity);
		if (!meshDrawComp) continue;

		// モデルデータを取得する
		auto modelData = engine->GetRenderContext()->GetModelStore()->GetModelData(meshDrawComp->handle);
		if (!modelData) continue;

		// スキニングインスタンスコンポーネントを取得する
		auto skinningInstanceComp = registry->GetComponent<SkinningInstanceComponent>(entity);
		auto skinningInstanceData = skinningInstanceStore->GetSkinningInstance(skinningInstanceComp->handle);
		if (!skinningInstanceData) continue;

		// スキンクラスターコンポーネントを取得
		auto skinClusterComp = registry->GetComponent<SkinClusterComponent>(entity);
		auto skinClusterData = skinClusterStore->GetSkinCluster(skinClusterComp->handle);
		if (!skinClusterData) continue;

		// スケルトンコンポーネントを取得する
		auto skeletonComp = registry->GetComponent<SkeletonComponent>(meshDrawComp->rootMeshEntity);
		auto skeletonData = skeletonStore->GetSkeletonData(skeletonComp->handle);
		if (!skeletonData) continue;

		

		auto animationComp = registry->GetComponent<AnimationComponent>(meshDrawComp->rootMeshEntity);

		Skeleton skeleton{};
		if (animationComp)
		{
			auto animationData = animationStore->GetAnimationData(animationComp->handle);
			if (animationData)
			{
				skeleton = ApplyBoneAnimation(skeletonData->skeleton, *animationData, animationComp->currentTime);
			}
			else
			{
				skeleton = skeletonData->skeleton;
			}
		}
		else
		{
			skeleton = skeletonData->skeleton;
		}

		// スケルトンの初期変換を設定する
		for (size_t i = 0; i < skeleton.joints.size(); ++i)
		{
			skeletonComp->pose.localTransforms[i] = skeleton.joints[i].initialTransform;
		}


		// スケルトン空間行列を更新する
		skeletonComp->pose.UpdateSkeletonSpaceMatrices(skeleton);

		// スキンクラスターのデータを更新する
		skinClusterData->Update(skeletonComp->pose, skinningInstanceData->GetInverseBindPoseMatrices(), frameIndex);

		// スキンクラスターの頂点リソースの状態を変更する
		skinClusterData->BarrierOutputVertexResource(commandList, frameIndex, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// スキニングインスタンスのデータを設定する
		skinningInstanceData->Register(commandList);

		// スキンクラスターのデータを設定する
		skinClusterData->Register(commandList, frameIndex);

		// ディスパッチ
		commandList->Dispatch(UINT(modelData->meshes[meshDrawComp->meshIndex].vertexBufferResource->GetNumVertex() + 1023) / 1024, 1, 1);

		// スキンクラスターの頂点リソースの状態を元に戻す
		skinClusterData->BarrierOutputVertexResource(commandList, frameIndex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}
}


/// @brief トランスフォームの更新
/// @param frameIndex 
void Detail::RenderSystem::UpdateTransform(int32_t frameIndex)
{
	auto  engine = Engine::GetInstance();
	if (!engine) return;

    auto resourceStore = engine->GetRenderContext()->GetResourceStore();
    if (!resourceStore) return;

	auto registry = engine->GetRegistryECS();
	if (!registry) return;

	for (auto entity : registry->GetComponentArray<TransformComponent>()->GetEntities())
	{
		// TransformComponentを取得する
		auto meshDrawComp = registry->GetComponent<MeshDrawComponent>(entity);
		if (!meshDrawComp) continue;

		// TransformConstantBufferComponentを取得する
		auto transformCB = Engine::GetInstance()->GetRegistryECS()->GetComponent<TransformConstantBufferComponent>(entity);
		if (!transformCB) continue;

		// TransformDataForGPUを計算して更新する
		TransformDataForGPU data;
		data.world = CalculateGlobalWorldMatrix(entity, meshDrawComp->rootMeshEntity);
		data.worldViewProjection = data.world * Engine::GetInstance()->GetRenderContext()->GetCameraSystem()->GetViewMatrix() * Engine::GetInstance()->GetRenderContext()->GetCameraSystem()->GetProjectionMatrix();
		data.worldInverseTranspose = data.world.Inverse().Transpose();

		// Transform用のリソースを更新する
        if (auto* cb = resourceStore->Get<ConstantBufferResource<TransformDataForGPU>>(transformCB->handle))
        {
            if (auto* mappedData = cb->GetData(frameIndex))
                *mappedData = data;
        }
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
	Entity parentEntity = kNullEntity;

	if (registry->HasComponent<HierarchyComponent>(entity))
	{
		auto hierarchy = registry->GetComponent<HierarchyComponent>(entity);
		if (hierarchy) parentEntity = hierarchy->parent;
	}
	else if (registry->HasComponent<JointHierarchyComponent>(entity))
	{
		auto hierarchy = registry->GetComponent<JointHierarchyComponent>(entity);
		if (hierarchy) parentEntity = hierarchy->parent;
	}

	if (parentEntity != kNullEntity)
	{
		Matrix4x4 parentWorld = CalculateGlobalWorldMatrix(parentEntity, rootEntity);
		return localWorld * parentWorld;
	}

	return localWorld;
}
