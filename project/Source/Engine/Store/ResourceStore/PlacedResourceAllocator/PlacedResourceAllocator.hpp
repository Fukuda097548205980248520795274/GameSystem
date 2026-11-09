#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <list>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace Detail
{
    // 配置リソースの割り当て情報
    struct PlacedAllocation
    {
        // ヒープのインデックス
        uint32_t heapIndex = 0;

        // 配置リソースのオフセット
        uint64_t offset = 0;

        // 配置リソースのサイズ
        uint64_t size = 0;

        /// @brief 配置リソースが有効かどうかを判定
        /// @return 
        bool IsValid() const { return size > 0; }
    };

    /// @brief 配置リソースの割り当てを管理するクラス
    class PlacedResourceAllocator
    {
    public:

        /// @brief コンストラクタ
        /// @param device 
        /// @param pageSizeInBytes 
        /// @param heapFlags 
        PlacedResourceAllocator(ID3D12Device* device, uint64_t pageSizeInBytes, D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES) : heapFlags_(heapFlags) { Initialize(device, pageSizeInBytes); }

        /// @brief デストラクタ
        ~PlacedResourceAllocator() = default;

        /// @brief メモリの割り当て（デフォルトはD3D12の標準である64KBアライメント）
        /// @param size 
        /// @param alignment 
        /// @return 
        PlacedAllocation Allocate(uint64_t size, uint64_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        /// @brief メモリの解放
        /// @param allocation 
        void Free(const PlacedAllocation& allocation);

        /// @brief ヒープを取得する
        /// @return 
        ID3D12Heap* GetHeap(uint32_t heapIndex) const { return pages_[heapIndex].heap.Get(); }

    private:

        /// @brief 初期化
        /// @param device 
        /// @param pageSizeInBytes 
        /// @param heapFlags 
        void Initialize(ID3D12Device* device, uint64_t pageSizeInBytes);

        /// @brief 新しいヒープページを追加する
        void AddNewPage();


    private:

        // 空きブロックの管理
        struct FreeBlock
        {
            // 空きブロックのオフセット
            uint64_t offset;

            // 空きブロックのサイズ
            uint64_t size;
        };

        // ヒープページの管理
        struct HeapPage
        {
            // ヒープの管理
            Microsoft::WRL::ComPtr<ID3D12Heap> heap;

            // 空きブロックのリスト（オフセット順にソートされている）
            std::list<FreeBlock> freeBlocks;
        };


        // デバイス
        ID3D12Device* device_ = nullptr;

        // ヒープのサイズ
        uint64_t pageSize_ = 0;

        // ヒープの配列
        std::vector<HeapPage> pages_;

        // メモリの割り当てと解放のスレッドセーフを確保するためのミューテックス
        std::mutex mutex_;

        // ヒープのフラグ（デフォルトはすべてのバッファとテクスチャを許可）
        D3D12_HEAP_FLAGS heapFlags_ = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    };
}
