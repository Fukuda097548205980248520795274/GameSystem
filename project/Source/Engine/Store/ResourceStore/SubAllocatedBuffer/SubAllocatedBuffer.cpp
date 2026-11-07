#include "SubAllocatedBuffer.hpp"

/// @brief コンストラクタ
/// @param device 
/// @param capacity 
Detail::SubAllocatedBuffer::SubAllocatedBuffer(ID3D12Device* device, uint64_t capacity) : allocator_(capacity), capacity_(capacity)
{
    assert(device);
    assert(capacity > 0);

    // CPUからデータを書き込む前提のためアップロードヒープを使用
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    // リソース設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = capacity;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // バッファリソースを生成
    HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource_));

    if (FAILED(hr))
    {
        throw std::runtime_error("巨大なバッファの生成に失敗しました。");
    }

    // 頻繁なMap/Unmapのオーバーヘッドを避けるため、永続的にマップしておく
    D3D12_RANGE readRange{ 0, 0 }; // CPUからの読み込みは行わない
    resource_->Map(0, &readRange, &mappedData_);
}

/// @brief デストラクタ
Detail::SubAllocatedBuffer::~SubAllocatedBuffer()
{
    if (resource_)
    {
        resource_->Unmap(0, nullptr);
        mappedData_ = nullptr;
    }
}

/// @brief メモリを確保する
/// @param size 
/// @param alignment 
/// @return 
Detail::SubAllocatedRegion Detail::SubAllocatedBuffer::Allocate(uint64_t size, uint64_t alignment)
{
    SubAllocation alloc = allocator_.Allocate(size, alignment);

    if (!alloc.IsValid())
        return SubAllocatedRegion{};

    SubAllocatedRegion region;
    region.allocation = alloc;

    // ベースアドレスにオフセットを足して、CPU/GPUそれぞれのアドレスを計算する
    region.gpuAddress = resource_->GetGPUVirtualAddress() + alloc.offset;
    if (mappedData_)
    {
        region.cpuAddress = static_cast<uint8_t*>(mappedData_) + alloc.offset;
    }

    return region;
}

/// @brief メモリを解放する
/// @param
void Detail::SubAllocatedBuffer::Free(const SubAllocatedRegion& region)
{
    if (region.IsValid())
    {
        allocator_.Free(region.allocation);
    }
}
