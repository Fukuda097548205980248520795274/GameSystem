#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <cassert>
#include <stdexcept>
#include "../SubAllocator/SubAllocator.hpp"

namespace Detail
{
    // @brief サブアロケーションを使用して管理されるバッファ
    class SubAllocatedBuffer;


    /// @brief サブアロケーションによって割り当てられたバッファ領域の情報
    struct SubAllocatedRegion
    {
        // アロケータの管理情報
        SubAllocation allocation;

        // GPUアクセス用アドレス
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;

        // CPUアクセス用アドレス（データ書き込み用）
        void* cpuAddress = nullptr;

        // この領域が属するSubAllocatedBufferのポインタ
        SubAllocatedBuffer* parentBuffer = nullptr;

        /// @brief 有効な割り当てかどうか
        /// @return 
        bool IsValid() const { return allocation.IsValid(); }
    };

    // @brief サブアロケータを使用して管理されるバッファ
    class SubAllocatedBuffer
    {
    public:

        /// @brief コンストラクタ
        /// @param device 
        /// @param capacity 
        SubAllocatedBuffer(ID3D12Device* device, uint64_t capacity);

        /// @brief デストラクタ
        ~SubAllocatedBuffer();

        /// @brief メモリを確保する
        /// @param size 
        /// @param alignment 
        /// @return 
        SubAllocatedRegion Allocate(uint64_t size, uint64_t alignment = 256);

        /// @brief メモリを解放する
        /// @param
        void Free(const SubAllocatedRegion& region);

        /// @brief 元となる巨大なリソースを取得する（SRVの生成等に使用）
        /// @return 
        ID3D12Resource* GetResource() const { return resource_.Get(); }


    private:

        // リソース
        Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

        // サブアロケータ
        SubAllocator allocator_;

        // 管理する総容量
        uint64_t capacity_ = 0;

        // 永続的にマップされたCPUアクセス用アドレス
        void* mappedData_ = nullptr;
    };
}
