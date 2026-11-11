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

/// @brief 初期化
/// @param device 
void Detail::ResourceStore::Initialize(ID3D12Device* device)
{
    if (!bufferAllocator_)
    {
        // デフォルトで64MBのページサイズとして初期化
        bufferAllocator_ = std::make_unique<BufferAllocator>(device, 1024 * 1024 * 64);
    }

    if (!placedAllocator_)
    {
        // デフォルトで256MBのページサイズとして初期化
        placedAllocator_ = std::make_unique<PlacedResourceAllocator>(device, 256 * 1024 * 1024);
    }
}
