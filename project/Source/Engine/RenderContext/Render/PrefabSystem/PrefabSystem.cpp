#include "PrefabSystem.hpp"
#include "Engine.hpp"
#include "Func/AnimationFunc/AnimationFunc.hpp"

/// @brief 初期化
/// @param device 
/// @param shaderCompiler 
void Detail::PrefabSystem::Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler)
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

    // プレハブ用の深度のみPSOを生成する
    depthOnlyPrefabPSO_ = std::make_unique<DepthOnlyPrefabPSO>(device, shaderCompiler);
}

/// @brief 更新処理
/// @param commandList 
/// @param frameIndex 
void Detail::PrefabSystem::Update(ID3D12GraphicsCommandList* commandList, int32_t frameIndex)
{
    // トランスフォームの更新
    UpdateTransform(frameIndex);
}

/// @brief モデルエンティティを作成する
/// @param modelHandle 
/// @param numInstances 
/// @return 
Entity Detail::PrefabSystem::CreateModelEntity(ModelHandle modelHandle, uint32_t numInstances)
{
    auto engine = Engine::GetInstance();
    auto renderContext = engine->GetRenderContext();
    auto device = renderContext->GetCore()->GetDevice();
    auto heap = renderContext->GetHeap();
    auto commandList = renderContext->GetCommand()->GetCommandList();
    auto resourceStore = renderContext->GetResourceStore();
    auto uploadBufferAllocator = resourceStore->GetUploadBufferAllocator();
    auto textureStore = renderContext->GetTextureStore();
    auto modelStore = renderContext->GetModelStore();
    uint32_t maxBufferCount = engine->GetMaxBufferCount();

    auto registry = engine->GetRegistryECS();
    if (!registry) return kNullEntity;

    // モデルデータを取得する
    auto modelData = modelStore->GetModelData(modelHandle);
    if (!modelData || modelData->nodes.empty()) return kNullEntity;

    // 親エンティティ（ルートモデルエンティティ）の作成
    Entity rootEntity = registry->CreateEntity();
    registry->AddComponent(rootEntity, TransformComponent());
    registry->AddComponent(rootEntity, HierarchyComponent());
    registry->AddComponent(rootEntity, JointHierarchyComponent());

    ModelComponent modelComp;
    modelComp.handle = modelHandle;
    registry->AddComponent(rootEntity, modelComp);

    // プレハブルートとしてコンポーネントを追加
    registry->AddComponent(rootEntity, PrefabComponent());
    registry->AddComponent(rootEntity, InstanceCountComponent());


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

                // マテリアルコンポーネント, UV変換コンポーネント, テクスチャコンポーネントを追加
                registry->AddComponent(entity, PSOComponent("Default_Prefab3D"));
                registry->AddComponent(entity, MaterialComponent());
                registry->AddComponent(entity, UVTransformComponent());
                registry->AddComponent(entity, TextureComponent(modelData->meshes[meshIdx].material.handle));

                // インスタンスデータ用の構造化バッファを作成して追加
                registry->AddComponent(entity, InstanceDataStructuredBufferComponent(
                    resourceStore->Create<StructuredBufferResource<InstanceDataForGPU>>(uploadBufferAllocator, device, heap, numInstances, maxBufferCount)));

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

/// @brief プレハブのインスタンスを作成する
/// @param sourcePrefab
/// @return 
Entity Detail::PrefabSystem::Instantiate(Entity prefabEntity)
{
    auto engine = Engine::GetInstance();
    if (!engine) return kNullEntity;

    auto registry = engine->GetRegistryECS();
    if (!registry) return kNullEntity;

    // 渡されたエンティティがプレハブかチェック
    if (!registry->HasComponent<PrefabComponent>(prefabEntity)) return kNullEntity;

    // インスタンス数をインクリメント（再帰の外で行う）
    if (auto instanceCountComp = registry->GetComponent<InstanceCountComponent>(prefabEntity))
    {
        instanceCountComp->count++;
    }

    // 再帰的にエンティティツリーを複製するラムダ関数
    std::function<Entity(Entity, Entity)> CloneEntityTree = [&](Entity source, Entity newParent) -> Entity
        {
            // 新しいエンティティを作成
            Entity newEntity = registry->CreateEntity();

            // 元のエンティティから全コンポーネントをコピー
            registry->CopyComponents(source, newEntity);

            // インスタンス側には不要なひな形用のコンポーネントを削除
            if (registry->HasComponent<PrefabComponent>(newEntity))
            {
                registry->RemoveComponent<PrefabComponent>(newEntity);
            }
            if (registry->HasComponent<InstanceCountComponent>(newEntity))
            {
                registry->RemoveComponent<InstanceCountComponent>(newEntity);
            }

            // ルートエンティティの場合のみ、プレハブのインスタンスであることを示すコンポーネントを追加
            if (newParent == kNullEntity)
            {
                PrefabInstanceComponent instanceComp(prefabEntity);
                registry->AddComponent(newEntity, instanceComp);
            }


            // 親子関係コンポーネントの更新
            if (auto hierarchy = registry->GetComponent<HierarchyComponent>(newEntity))
            {
                hierarchy->parent = newParent;
                hierarchy->children.clear(); // 古い子IDをクリア
            }

            // ポインタの無効化を防ぐため、コピー元の子エンティティリストを値コピーしておく
            std::vector<Entity> sourceChildren;
            if (auto sourceHierarchy = registry->GetComponent<HierarchyComponent>(source))
            {
                sourceChildren = sourceHierarchy->children;
            }

            // 子を再帰的に複製し、新しい子IDリストをローカル変数に集める
            std::vector<Entity> newChildren;
            for (Entity sourceChild : sourceChildren)
            {
                Entity newChild = CloneEntityTree(sourceChild, newEntity);
                newChildren.push_back(newChild);
            }

            // 再帰呼び出し（メモリ再確保）が全て終わった後に、再度コンポーネントを取得して書き込む
            if (!newChildren.empty())
            {
                if (auto hierarchy = registry->GetComponent<HierarchyComponent>(newEntity))
                {
                    hierarchy->children = newChildren;
                }
            }


            // ジョイント階層コンポーネントの更新
            if (auto jointHierarchy = registry->GetComponent<JointHierarchyComponent>(newEntity))
            {
                jointHierarchy->parent = newParent;
                jointHierarchy->children.clear();
            }

            // こちらも同様に値コピーと再取得を行う
            std::vector<Entity> sourceJointChildren;
            if (auto sourceJointHierarchy = registry->GetComponent<JointHierarchyComponent>(source))
            {
                sourceJointChildren = sourceJointHierarchy->children;
            }

            std::vector<Entity> newJointChildren;
            for (Entity sourceChild : sourceJointChildren)
            {
                Entity newChild = CloneEntityTree(sourceChild, newEntity);
                newJointChildren.push_back(newChild);
            }

            if (!newJointChildren.empty())
            {
                if (auto jointHierarchy = registry->GetComponent<JointHierarchyComponent>(newEntity))
                {
                    jointHierarchy->children = newJointChildren;
                }
            }


            return newEntity;
        };

    // ルートから再帰的な複製を開始 (親は kNullEntity)
    Entity instanceRootEntity = CloneEntityTree(prefabEntity, kNullEntity);

    return instanceRootEntity;
}

/// @brief 深度値のみの描画を実行する
/// @param commandList 
/// @param frameIndex 
void Detail::PrefabSystem::ExecuteDepthPrePass(ID3D12GraphicsCommandList* commandList, int32_t frameIndex)
{
    auto engine = Engine::GetInstance();
    if (!engine) return;

    auto renderContext = engine->GetRenderContext();
    if (!renderContext) return;

    auto resourceStore = renderContext->GetResourceStore();
    if (!resourceStore) return;

    auto modelStore = renderContext->GetModelStore();
    if (!modelStore) return;

    auto registry = engine->GetRegistryECS();
    if (!registry) return;

    // プレハブごとに深度描画
    for (Entity rootEntity : registry->GetComponentArray<PrefabComponent>()->GetEntities())
    {
        auto pass = registry->GetComponent<PrefabComponent>(rootEntity);
        auto instanceCountComp = registry->GetComponent<InstanceCountComponent>(rootEntity);
        auto modelComp = registry->GetComponent<ModelComponent>(rootEntity);

        if (!pass || !instanceCountComp || instanceCountComp->count == 0 || !modelComp) continue;

        auto modelData = modelStore->GetModelData(modelComp->handle);
        if (!modelData) continue;

        // 深度のみのPSOを登録する
        depthOnlyPrefabPSO_->Register(commandList);

        // 再帰的にメッシュノードを走査
        std::function<void(Entity)> DrawDepthNodes = [&](Entity currentEntity)
            {
                auto instanceDataComp = registry->GetComponent<InstanceDataStructuredBufferComponent>(currentEntity);
                auto nodeComp = registry->GetComponent<NodeComponent>(currentEntity);

                if (instanceDataComp && nodeComp)
                {
                    const auto& node = modelData->nodes[nodeComp->index];
                    if (node.meshIndex.has_value())
                    {
                        int32_t meshIdx = node.meshIndex.value();

                        modelStore->Register(commandList, modelComp->handle, meshIdx);

                        // Transform (InstanceData) 用のリソースをバインド (DepthOnlyPSOのレジスタ0番を想定)
                        auto instanceDataBuffer = resourceStore->Get<StructuredBufferResource<InstanceDataForGPU>>(instanceDataComp->handle);
                        if (instanceDataBuffer) instanceDataBuffer->RegisterGraphics(commandList, 0, frameIndex);

                        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                        commandList->DrawIndexedInstanced(modelData->meshes[meshIdx].indexBufferResource->GetNumIndex(), instanceCountComp->count, 0, 0, 0);
                    }
                }

                // 子ノードへ再帰
                if (auto hierarchy = registry->GetComponent<HierarchyComponent>(currentEntity))
                {
                    for (Entity child : hierarchy->children) DrawDepthNodes(child);
                }
                if (auto jointHierarchy = registry->GetComponent<JointHierarchyComponent>(currentEntity))
                {
                    for (Entity child : jointHierarchy->children) DrawDepthNodes(child);
                }
            };

        DrawDepthNodes(rootEntity);
    }
}

/// @brief すべての3D描画を実行する
/// @param commandList 
/// @param priority 
void Detail::PrefabSystem::ExecuteAll(ID3D12GraphicsCommandList* commandList, int priority)
{
    // priority が負の値の場合は何もしない
    if (priority < 0) return;

    auto engine = Engine::GetInstance();
    if (!engine) return;

    auto renderContext = engine->GetRenderContext();
    if (!renderContext) return;

    auto resourceStore = renderContext->GetResourceStore();
    if (!resourceStore) return;

    auto textureStore = renderContext->GetTextureStore();
    if (!textureStore) return;

    auto lightSystem = renderContext->GetLightSystem();
    if (!lightSystem) return;

    auto modelStore = renderContext->GetModelStore();
    if (!modelStore) return;

    // ECSレジストリを取得する
    auto registry = engine->GetRegistryECS();
    if (!registry) return;

    // PrefabComponentの配列を取得する（プレハブのルートエンティティ）
    auto* passArray = registry->GetComponentArray<PrefabComponent>();
    const auto& entities = passArray->GetEntities();

    // 今のフレームインデックスを取得する
    int32_t frameIndex = engine->GetFrameIndex();

    // priority に基づいてソートするためのペアのベクターを作成
    std::vector<std::pair<Entity, int>> sortedPasses;
    for (Entity entity : entities)
    {
        auto pass = registry->GetComponent<PrefabComponent>(entity);
        auto instanceCountComp = registry->GetComponent<InstanceCountComponent>(entity);

        // priority が一致しない場合、またはインスタンスがない場合はスキップ
        if (!(pass && instanceCountComp && pass->priority == priority)) continue;
        if (instanceCountComp->count == 0) continue;

        sortedPasses.push_back({ entity, pass->priority });
    }

    // ソートされたレンダーパスが空の場合は何もしない
    if (sortedPasses.empty()) return;

    // priority に基づいて昇順でソート（数値が小さいほど先に描画）
    std::sort(sortedPasses.begin(), sortedPasses.end(), [](const auto& a, const auto& b) {return a.second < b.second;});

    // ソートされたプレハブを実行する
    for (const auto& [rootEntity, p] : sortedPasses)
    {
        auto instanceCountComp = registry->GetComponent<InstanceCountComponent>(rootEntity);
        auto modelComp = registry->GetComponent<ModelComponent>(rootEntity);
        if (!instanceCountComp || !modelComp) continue;

        // モデルデータを取得する
        auto modelData = modelStore->GetModelData(modelComp->handle);
        if (!modelData) continue;

        // スケルトンコンポーネントはルートに付与されている
        auto skeletonComp = registry->GetComponent<SkeletonComponent>(rootEntity);

        // プレハブ内のメッシュノードを再帰的に走査して描画する関数
        std::function<void(Entity)> DrawMeshNodes = [&](Entity currentEntity)
            {
                auto nodeComp = registry->GetComponent<NodeComponent>(currentEntity);
                auto instanceDataComp = registry->GetComponent<InstanceDataStructuredBufferComponent>(currentEntity);
                auto material = registry->GetComponent<MaterialComponent>(currentEntity);
                auto uvTransform = registry->GetComponent<UVTransformComponent>(currentEntity);
                auto texture = registry->GetComponent<TextureComponent>(currentEntity);

                // メッシュ描画に必要なコンポーネントが揃っているか確認
                if (nodeComp && instanceDataComp && material && uvTransform && texture)
                {
                    const auto& node = modelData->nodes[nodeComp->index];
                    if (node.meshIndex.has_value())
                    {
                        int32_t meshIdx = node.meshIndex.value();
                        auto psoComp = registry->GetComponent<PSOComponent>(currentEntity);

                        // PSOを取得する
                        auto psoItem = psoEditor_->GetPSOItem(psoComp->psoName);
                        if (psoItem && psoItem->pso)
                        {
                            // PSOとルートシグネチャを設定する
                            commandList->SetPipelineState(psoItem->pso.Get());
                            commandList->SetGraphicsRootSignature(psoItem->rootSig.Get());

                            auto skinningComp = registry->GetComponent<SkinningInstanceComponent>(currentEntity);
                            auto skinClusterComp = registry->GetComponent<SkinClusterComponent>(currentEntity);

                            // スキニングコンポーネントとスキンクラスターが存在する場合
                            if (skeletonComp && skinningComp && skinClusterComp)
                            {
                                auto skinClusterStore = engine->GetRenderContext()->GetSkinClusterStore();
                                if (skinClusterStore)
                                {
                                    auto skinClusterData = skinClusterStore->GetSkinCluster(skinClusterComp->handle);
                                    if (skinClusterData)
                                    {
                                        skinClusterData->RegisterVertex(commandList, frameIndex);
                                        modelStore->RegisterIndexBuffer(commandList, modelComp->handle, meshIdx);
                                    }
                                }
                            }
                            else
                            {
                                // スキニングコンポーネントが存在しない場合は、通常の描画処理を行う
                                modelStore->Register(commandList, modelComp->handle, meshIdx);
                            }

                            // PSOの要求に従って動的にパラメータをバインドする（現状維持）
                            for (UINT i = 0; i < psoItem->desc.rootParameters.size(); ++i)
                            {
                                const auto& param = psoItem->desc.rootParameters[i];
                                switch (param.binding)
                                {
                                case RenderBinding::InstaceData:
                                    if (auto instanceDataBuffer = resourceStore->Get<StructuredBufferResource<InstanceDataForGPU>>(instanceDataComp->handle))
                                        instanceDataBuffer->RegisterGraphics(commandList, i, frameIndex);
                                    break;
                                case RenderBinding::Camera:
                                    if (auto cbResource = lightSystem->GetCameraDataBuffer())
                                        cbResource->RegisterGraphics(commandList, i, frameIndex);
                                    break;
                                case RenderBinding::NumLights:
                                    if (auto cbResource = lightSystem->GetNumLightDataBuffer())
                                        cbResource->RegisterGraphics(commandList, i, frameIndex);
                                    break;
                                case RenderBinding::DirectionalLight:
                                    if (auto cbResource = lightSystem->GetDirectionalLightDataBuffer())
                                        cbResource->RegisterGraphics(commandList, i, frameIndex);
                                    break;
                                case RenderBinding::PointLight:
                                    if (auto cbResource = lightSystem->GetPointLightDataBuffer())
                                        cbResource->RegisterGraphics(commandList, i, frameIndex);
                                    break;
                                case RenderBinding::SpotLight:
                                    if (auto cbResource = lightSystem->GetSpotLightDataBuffer())
                                        cbResource->RegisterGraphics(commandList, i, frameIndex);
                                    break;
                                case RenderBinding::LightCullingTitle:
                                    if (auto cbResource = lightSystem->GetTileLightIndexBuffer())
                                        cbResource->RegisterGraphicsSRV(commandList, i, frameIndex);
                                    break;
                                case RenderBinding::Texture:
                                    commandList->SetGraphicsRootDescriptorTable(i, textureStore->GetSrvHandle(texture->handle).gpuHandle);
                                    break;
                                }
                            }

                            // プリミティブトポロジーを設定する
                            if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                            else if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
                            else if (psoItem->desc.primitiveTopology == D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT) commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

                            // メッシュインデックスを使用してドローコール（インスタンス描画）
                            commandList->DrawIndexedInstanced(modelData->meshes[meshIdx].indexBufferResource->GetNumIndex(), instanceCountComp->count, 0, 0, 0);
                        }
                    }
                }

                // 子ノードへ再帰
                if (auto hierarchy = registry->GetComponent<HierarchyComponent>(currentEntity))
                {
                    for (Entity child : hierarchy->children) DrawMeshNodes(child);
                }
                if (auto jointHierarchy = registry->GetComponent<JointHierarchyComponent>(currentEntity))
                {
                    for (Entity child : jointHierarchy->children) DrawMeshNodes(child);
                }
            };

        // ルートから走査開始
        DrawMeshNodes(rootEntity);
    }
}

/// @brief トランスフォームの更新
/// @param frameIndex 
void Detail::PrefabSystem::UpdateTransform(int32_t frameIndex)
{
    auto engine = Engine::GetInstance();
    if (!engine) return;

    auto renderContext = engine->GetRenderContext();
    if (!renderContext) return;

    auto resourceStore = renderContext->GetResourceStore();
    if (!resourceStore) return;

    auto cameraSystem = renderContext->GetCameraSystem();
    if (!cameraSystem) return;

    auto registry = engine->GetRegistryECS();
    if (!registry) return;

    // カメラのビュー・プロジェクション行列を取得
    Matrix4x4 viewMatrix = cameraSystem->GetViewMatrix();
    Matrix4x4 projMatrix = cameraSystem->GetProjectionMatrix();
    Matrix4x4 viewProj = viewMatrix * projMatrix;

    // インスタンスのルートから再帰的に同名ノードを検索するラムダ関数
    std::function<Entity(Entity, const std::string&)> FindNodeInInstance = [&](Entity current, const std::string& name) -> Entity
        {
            auto nodeC = registry->GetComponent<NodeComponent>(current);
            if (nodeC && nodeC->name == name) return current;

            if (auto hierarchy = registry->GetComponent<HierarchyComponent>(current))
            {
                for (Entity child : hierarchy->children)
                {
                    Entity found = FindNodeInInstance(child, name);
                    if (found != kNullEntity) return found;
                }
            }
            if (auto jointHierarchy = registry->GetComponent<JointHierarchyComponent>(current))
            {
                for (Entity child : jointHierarchy->children)
                {
                    Entity found = FindNodeInInstance(child, name);
                    if (found != kNullEntity) return found;
                }
            }
            return kNullEntity;
        };

    // 1. プレハブ（雛形）のルートエンティティを反復
    for (Entity prefabRoot : registry->GetComponentArray<PrefabComponent>()->GetEntities())
    {
        // プレハブコンポーネントとインスタンスカウントを取得
        auto prefabComp = registry->GetComponent<PrefabComponent>(prefabRoot);
        auto instanceCountComp = registry->GetComponent<InstanceCountComponent>(prefabRoot);
        if (!prefabComp || !instanceCountComp || instanceCountComp->count == 0) continue;

        // このプレハブに属する全インスタンス（ルート）を収集
        std::vector<Entity> instanceRoots;
        for (Entity instEntity : registry->GetComponentArray<PrefabInstanceComponent>()->GetEntities())
        {
            auto instComp = registry->GetComponent<PrefabInstanceComponent>(instEntity);
            if (instComp && instComp->sourcePrefab == prefabRoot)
            {
                instanceRoots.push_back(instEntity);
            }
        }

        // 2. プレハブ内のメッシュエンティティを検索するラムダ関数
        std::function<void(Entity, const std::string&)> ProcessMeshNodes = [&](Entity currentPrefabNode, const std::string& nodeName)
            {
                // メッシュノードなら構造化バッファに書き込む
                if (auto structBufferComp = registry->GetComponent<InstanceDataStructuredBufferComponent>(currentPrefabNode))
                {
                    if (auto cb = resourceStore->Get<StructuredBufferResource<InstanceDataForGPU>>(structBufferComp->handle))
                    {
                        if (auto mappedData = cb->GetData(frameIndex))
                        {
                            // 各インスタンスごとに該当ノードのワールド行列を計算して配列に格納
                            for (size_t i = 0; i < instanceRoots.size(); ++i)
                            {
                                Entity instRoot = instanceRoots[i];

                                // インスタンスの階層ツリーから同名のノードを検索
                                Entity targetInstNode = FindNodeInInstance(instRoot, nodeName);

                                if (targetInstNode != kNullEntity)
                                {
                                    // 1. 構造体をゼロクリア初期化して生成（不要なゴミデータが入るのを防ぐ）
                                    InstanceDataForGPU data{};

                                    // 2. ワールド行列やビュープロジェクション行列を計算して代入
                                    data.world = CalculateGlobalWorldMatrix(targetInstNode, instRoot);
                                    data.worldViewProjection = data.world * viewProj;
                                    data.worldInverseTranspose = data.world.Inverse().Transpose();

                                    // 3. その他必要なマテリアル情報等があればここで代入
                                    if (auto matComp = registry->GetComponent<MaterialComponent>(targetInstNode))
                                        data.color = matComp->color;
                                    else
                                        data.color = { 1.0f, 1.0f, 1.0f, 1.0f };

                                    data.textureIndex = 0; // ルートパラメータ仕様に合わせる場合

                                    // 4. ストラクチャードバッファのマップドメモリにデータを書き込む
                                    mappedData[i] = data;
                                }
                            }
                        }
                    }
                }

                // 子ノードへ再帰
                if (auto hierarchy = registry->GetComponent<HierarchyComponent>(currentPrefabNode))
                {
                    for (Entity child : hierarchy->children)
                    {
                        if (auto nodeComp = registry->GetComponent<NodeComponent>(child))
                        {
                            ProcessMeshNodes(child, nodeComp->name);
                        }
                    }
                }

                if (auto jointHierarchy = registry->GetComponent<JointHierarchyComponent>(currentPrefabNode))
                {
                    for (Entity child : jointHierarchy->children)
                    {
                        if (auto nodeComp = registry->GetComponent<NodeComponent>(child))
                        {
                            ProcessMeshNodes(child, nodeComp->name);
                        }
                    }
                }
            };

        // ルートから走査開始
        if (auto rootNodeComp = registry->GetComponent<NodeComponent>(prefabRoot))
        {
            ProcessMeshNodes(prefabRoot, rootNodeComp->name);
        }
        else
        {
            // ルート直下の子から走査
            if (auto hierarchy = registry->GetComponent<HierarchyComponent>(prefabRoot))
            {
                for (Entity child : hierarchy->children)
                {
                    if (auto nodeComp = registry->GetComponent<NodeComponent>(child))
                        ProcessMeshNodes(child, nodeComp->name);
                }
            }
        }
    }
}

/// @brief グローバルなワールド行列を計算する
/// @param entity 
/// @param isAnimation 
/// @param meshIndex 
/// @return 
Matrix4x4 Detail::PrefabSystem::CalculateGlobalWorldMatrix(Entity entity, Entity rootEntity)
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
