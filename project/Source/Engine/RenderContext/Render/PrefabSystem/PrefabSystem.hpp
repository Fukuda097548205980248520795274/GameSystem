#pragma once
#include "ECS/EntityECS/EntityECS.hpp"
#include "DepthOnlyPrefabPSO/DepthOnlyPrefabPSO.hpp"

namespace Detail
{
    class PrefabSystem
    {
    public:

        /// @brief コンストラクタ
        /// @param device 
        /// @param shaderCompiler 
        PrefabSystem(ID3D12Device* device, ShaderCompiler* shaderCompiler) { Initialize(device, shaderCompiler); }



    private:


        /// @brief 初期化
        /// @param device 
        /// @param shaderCompiler 
        void Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler);


    private:

        std::unique_ptr<DepthOnlyPrefabPSO> depthOnlyPrefabPSO_ = nullptr;
    };
}
