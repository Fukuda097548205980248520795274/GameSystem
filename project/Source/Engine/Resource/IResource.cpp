#include "IResource.hpp"

/// @brief 現在のリソース状態を取得する
/// @param bufferIndex 
/// @return 
D3D12_RESOURCE_STATES Detail::IResource::GetState(uint32_t bufferIndex) const
{
    // バッファインデックスが範囲内であれば現在のステートを返す
    if (bufferIndex < currentStates_.size())
        return currentStates_[bufferIndex];

    // デフォルトのリソースステートを返す
    return D3D12_RESOURCE_STATE_COMMON;
}
