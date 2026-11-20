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

            // これがプレハブのインスタンスであることを示すコンポーネントを追加
            PrefabInstanceComponent instanceComp(prefabEntity);
            registry->AddComponent(newEntity, instanceComp);


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

/// @brief トランスフォームの更新
/// @param frameIndex 
void Detail::PrefabSystem::UpdateTransform(int32_t frameIndex)
{
    
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
