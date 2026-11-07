#include "ResourceStore.hpp"

/// @brief ハンドルを指定してリソースを解放（空きキューに戻す）
/// @param handle 
void Detail::ResourceStore::Release(int32_t handle)
{
    if (handle >= 0 && handle < resources_.size() && resources_[handle].resource)
    {
        freeHandles_[resources_[handle].type].push(handle);
    }
}
