#include "ResourceStore.hpp"

/// @brief 更新処理
/// @param maxBufferCount 
/// @param frameIndex 
void Detail::ResourceStore::Update(uint32_t maxBufferCount, uint32_t frameIndex)
{
    // 破棄を遅延処理する
    while (!deferredFreeQueue_.empty())
    {
        int32_t requestedFrame = deferredFreeQueue_.front().requestedFrameIndex;

        // 解放要求がmaxBufferCountフレーム以上前であれば解放する
        if (frameIndex - requestedFrame >= maxBufferCount)
        {
            // アロケータから解放
            uploadBufferAllocator_->Free(deferredFreeQueue_.front().region);
            deferredFreeQueue_.pop();
        }
        else
        {
            // 先頭がまだ解放できないなら以降も解放不可
            break;
        }
    }
}

/// @brief UPLOADヒープ領域の遅延解放を予約する
/// @param region 
/// @param frameIndex 
void Detail::ResourceStore::FreeUploadRegionDeferred(const SubAllocatedRegion& region, uint32_t frameIndex)
{
    if (region.IsValid())
    {
        deferredFreeQueue_.push({ region, frameIndex });
    }
}

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
    // UPLOADヒープ用にデフォルトで64MBのページサイズとして初期化
    uploadBufferAllocator_ = std::make_unique<BufferAllocator>(device, 1024 * 1024 * 64, D3D12_HEAP_TYPE_UPLOAD);

    // DEFAULTヒープ用にデフォルトで64MBのページサイズとして初期化
    defaultBufferAllocator_ = std::make_unique<BufferAllocator>(device, 1024 * 1024 * 64, D3D12_HEAP_TYPE_DEFAULT);

    // デフォルトで256MBのページサイズとして初期化
    placedAllocator_ = std::make_unique<PlacedResourceAllocator>(device, 256 * 1024 * 1024);
}
