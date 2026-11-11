#pragma once
#include "Matrix/Matrix4x4/Matrix4x4.hpp"
#include "ECS/EntityECS/EntityECS.hpp"
#include "ShadowMapPSO/ShadowMapPSO.hpp"
#include "ShadowMapPrefabPSO/ShadowMapPrefabPSO.hpp"

namespace Detail
{
    class ShadowRenderSystem
    {
    public:

        /// @brief コンストラクタ
        /// @param device 
        /// @param shaderCompiler 
        ShadowRenderSystem(ID3D12Device* device, ShaderCompiler* shaderCompiler) { Initialize(device, shaderCompiler); }

        /// @brief 更新処理
        /// @param commandList 
        /// @param frameIndex 
        void Update(ID3D12GraphicsCommandList* commandList, int32_t frameIndex);


    private:

        /// @brief 初期化
        /// @param device 
        /// @param shaderCompiler 
        void Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler);

        /// @brief シャドウマップパスを実行する
        /// @param commandList 
        /// @param frameIndex 
        void ExecuteShadowMapPass(ID3D12GraphicsCommandList* commandList, int32_t frameIndex);


    private:

        /// @brief シャドウマップPSO
        std::unique_ptr<ShadowMapPSO> shadowMapPso_ = nullptr;

        /// @brief シャドウマップPrefabPSO
        std::unique_ptr<ShadowMapPrefabPSO> shadowMapPrefabPso_ = nullptr;
    };
}
