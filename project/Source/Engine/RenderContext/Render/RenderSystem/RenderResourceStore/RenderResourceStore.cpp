#include "RenderResourceStore.h"

/// @brief 定数バッファのGPU仮想アドレスを取得する
/// @param cbHandle 
/// @param frameIndex 
/// @return 
D3D12_GPU_VIRTUAL_ADDRESS Detail::RenderResourceStore::GetGPUVirtualAddress(int32_t cbHandle, uint32_t frameIndex)
{
    if (cbHandle >= 0 && cbHandle < constantBuffers_.size() && constantBuffers_[cbHandle])
    {
        // 仮想アドレス取得のためには、IConstantBufferResource側に対応メソッドが必要です（後述）
        return constantBuffers_[cbHandle]->GetGPUVirtualAddress(frameIndex);
    }

    return 0;
}

/// @brief 定数バッファを解放する
/// @param cbHandle 
void Detail::RenderResourceStore::ReleaseConstantBuffer(int32_t cbHandle)
{
    // 範囲チェックを行い、リソースを解放(nullptrにする)
    if (cbHandle >= 0 && cbHandle < constantBuffers_.size())
    {
        constantBuffers_[cbHandle].reset();
    }
}

/// @brief 定数バッファを取得する
/// @param cbHandle 
/// @return 
Detail::IConstantBufferResource* Detail::RenderResourceStore::GetConstantBuffer(int32_t cbHandle)
{
    if (cbHandle >= 0 && cbHandle < constantBuffers_.size())
    {
        return constantBuffers_[cbHandle].get();
    }

    return nullptr;
}