#pragma once
#include "ECS/EntityECS/EntityECS.hpp"
#include "DepthOnlyPrefabPSO/DepthOnlyPrefabPSO.hpp"
#include "Store/ModelStore/ModelData.hpp"

namespace Detail
{
    // brief ECSレジストリ
    class PSOEditor;

    ///brief プレハブシステム
    class PrefabSystem
    {
    public:

        /// @brief コンストラクタ
        /// @param device 
        /// @param shaderCompiler 
        PrefabSystem(ID3D12Device* device, ShaderCompiler* shaderCompiler) { Initialize(device, shaderCompiler); }

        /// @brief 更新処理
        /// @param commandList 
        /// @param frameIndex 
        void Update(ID3D12GraphicsCommandList* commandList, int32_t frameIndex);

        /// @brief モデルエンティティを作成する
        /// @param modelHandle 
        /// @param numInstances 
        /// @return 
        Entity CreateModelEntity(ModelHandle modelHandle, uint32_t numInstances);

        /// @brief プレハブのインスタンスを作成する
        /// @param sourcePrefab 
        /// @return 
        Entity Instantiate(Entity prefabEntity);

        /// @brief すべてのインスタンスを削除する
        void DestroyAllInstances();

        /// @brief 深度値のみの描画を実行する
        /// @param commandList 
        /// @param frameIndex 
        void ExecuteDepthPrePass(ID3D12GraphicsCommandList* commandList, int32_t frameIndex = 0);

        /// @brief すべての3D描画を実行する
        /// @param commandList 
        /// @param priority 
        void ExecuteAll(ID3D12GraphicsCommandList* commandList, int priority);



    private:


        /// @brief 初期化
        /// @param device 
        /// @param shaderCompiler 
        void Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler);

        /// @brief トランスフォームの更新
        /// @param frameIndex 
        void UpdateTransform(int32_t frameIndex);

        /// @brief グローバルなワールド行列を計算する
        /// @param entity 
        /// @param isAnimation 
        /// @param meshIndex 
        /// @return 
        Matrix4x4 CalculateGlobalWorldMatrix(Entity entity, Entity rootEntity);


    private:

        // brief 深度のみPSO
        std::unique_ptr<DepthOnlyPrefabPSO> depthOnlyPrefabPSO_ = nullptr;

        /// @brief PSOエディタ
        PSOEditor* psoEditor_ = nullptr;
    };
}
