#include "BufferAllocator.hpp"

/// @brief メモリを確保する
/// @param size 
/// @param alignment 
/// @return 
Detail::SubAllocatedRegion Detail::BufferAllocator::Allocate(uint64_t size, uint64_t alignment)
{
    // 既存のページから空き領域を探す
    for (auto& page : pages_)
    {
        SubAllocatedRegion region = page->Allocate(size, alignment);
        if (region.IsValid())
        {
            region.parentBuffer = page.get();
            return region;
        }
    }
    
    // 既存ページに十分な空きがない場合、新しいページを生成して確保
    auto newPage = std::make_unique<SubAllocatedBuffer>(device_, pageSize_, heapType_);
    SubAllocatedRegion region = newPage->Allocate(size, alignment);
    region.parentBuffer = newPage.get();

    pages_.push_back(std::move(newPage));
    return region;
}

/// @brief メモリを解放する
/// @param region 
void Detail::BufferAllocator::Free(const SubAllocatedRegion& region)
{
    if (region.IsValid() && region.parentBuffer)
    {
        region.parentBuffer->Free(region);
    }
}
