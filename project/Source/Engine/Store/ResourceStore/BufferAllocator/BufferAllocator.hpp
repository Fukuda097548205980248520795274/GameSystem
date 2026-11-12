#pragma once
#include <vector>
#include <memory>
#include "../SubAllocatedBuffer/SubAllocatedBuffer.hpp"

namespace Detail
{
    class BufferAllocator
    {
    public:

        /// @brief コンストラクタ
        /// @param device 
        /// @param pageSize 
        /// @param heapType 
        BufferAllocator(ID3D12Device* device, uint64_t pageSize = 1024 * 1024 * 64, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD) : device_(device), pageSize_(pageSize), heapType_(heapType) {}

        /// @brief メモリを確保する
        /// @param size 
        /// @param alignment 
        /// @return 
        SubAllocatedRegion Allocate(uint64_t size, uint64_t alignment = 256);

        /// @brief メモリを解放する
        /// @param region 
        void Free(const SubAllocatedRegion& region);


    private:

        // デバイス
        ID3D12Device* device_ = nullptr;

        // ヒープタイプ
        D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_UPLOAD;

        // 管理するページのサイズ
        uint64_t pageSize_ = 0;

        // 管理するページのリスト
        std::vector<std::unique_ptr<SubAllocatedBuffer>> pages_;
    };
}
