#include "ResourceStore.hpp"

/// @brief ハンドルを指定してリソースを解放（空きキューに戻す）
/// @param handle 
void Detail::ResourceStore::Release(int32_t handle)
{
    // ハンドルが有効かチェック
    if (IsValid(handle))
    {
        // リソースを確実に破棄してメモリを解放する
        resources_[handle].resource.reset();
        resources_[handle].type = std::type_index(typeid(void));

        // 空きハンドルとして再利用キューに登録
        freeHandles_.push(handle);
    }
}

/// @brief 初期化
/// @param device 
void Detail::ResourceStore::Initialize(ID3D12Device* device)
{
    // デフォルトで64MBのページサイズとして初期化
    bufferAllocator_ = std::make_unique<BufferAllocator>(device, 1024 * 1024 * 64);

    // デフォルトで256MBのページサイズとして初期化
    placedAllocator_ = std::make_unique<PlacedResourceAllocator>(device, 256 * 1024 * 1024);
}
