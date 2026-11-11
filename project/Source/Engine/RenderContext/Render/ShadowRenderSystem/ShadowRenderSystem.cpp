#include "ShadowRenderSystem.hpp"
#include "Engine.hpp"

/// @brief 初期化
/// @param device 
/// @param shaderCompiler 
void Detail::ShadowRenderSystem::Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler)
{
    assert(device);
    assert(shaderCompiler);

    // シャドウマップPSOを生成する
    shadowMapPso_ = std::make_unique<ShadowMapPSO>(device, shaderCompiler);

    // シャドウマップPrefabPSOを生成する
    shadowMapPrefabPso_ = std::make_unique<ShadowMapPrefabPSO>(device, shaderCompiler);
}

/// @brief 更新処理
/// @param commandList 
/// @param frameIndex 
void Detail::ShadowRenderSystem::Update(ID3D12GraphicsCommandList* commandList, int32_t frameIndex)
{
    assert(commandList);


}


/// @brief シャドウマップパスを実行する
/// @param commandList 
/// @param frameIndex 
void Detail::ShadowRenderSystem::ExecuteShadowMapPass(ID3D12GraphicsCommandList* commandList, int32_t frameIndex)
{

}
