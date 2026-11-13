#include "PrefabSystem.hpp"
#include "Engine.hpp"

/// @brief 初期化
/// @param device 
/// @param shaderCompiler 
void Detail::PrefabSystem::Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler)
{
    // nullptrチェック
    assert(device);
    assert(shaderCompiler);

    // プレハブ用の深度のみPSOを生成する
    depthOnlyPrefabPSO_ = std::make_unique<DepthOnlyPrefabPSO>(device, shaderCompiler);
}
