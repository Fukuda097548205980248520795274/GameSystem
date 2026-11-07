#pragma once
#include <cstdint>
#include <list>
#include <algorithm>

namespace Detail
{
    // @brief サブアロケータの割り当て情報
    struct SubAllocation
    {
        // 確保されたオフセット（バイト単位）
        uint64_t offset = 0;

        // 確保されたサイズ（バイト単位）
        uint64_t size = 0;

        /// @brief 有効な割り当てかどうか
        /// @return 
        bool IsValid() const { return size > 0; }
    };

    // @brief サブアロケータ
    class SubAllocator
    {
    public:

        /// @brief コンストラクタ
        /// @param capacity 
        explicit SubAllocator(uint64_t capacity);

        /// @brief メモリを確保する
        /// @param size 
        /// @param alignment 
        /// @return 
        SubAllocation Allocate(uint64_t size, uint64_t alignment);

        /// @brief メモリを解放し、必要に応じて隣接する空きブロックと結合（マージ）する
        /// @param allocation
        void Free(const SubAllocation& allocation);

        /// @brief 管理している総容量を取得
        /// @return 
        uint64_t GetCapacity() const { return capacity_; }

    private:

        /// @brief 空き領域ブロック
        struct FreeBlock
        {
            uint64_t offset;
            uint64_t size;
        };

        /// @brief 指定したイテレータのブロックを前後のブロックと結合する
        /// @param it 
        void MergeBlocks(std::list<FreeBlock>::iterator it);


    private:

        // 管理する総容量
        uint64_t capacity_ = 0;

        // 空きブロックのリスト（オフセット順にソートされていることを前提とする）
        std::list<FreeBlock> freeBlocks_;
    };
}
