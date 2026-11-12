#include "SubAllocatedBuffer.hpp"

/// @brief コンストラクタ
/// @param device 
/// @param capacity 
/// @param heapType 
Detail::SubAllocatedBuffer::SubAllocatedBuffer(ID3D12Device* device, uint64_t capacity, D3D12_HEAP_TYPE heapType) : allocator_(capacity), capacity_(capacity), heapType_(heapType)
{
    assert(device);
    assert(capacity > 0);

    // CPUからデータを書き込む前提のためアップロードヒープを使用
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = heapType_;
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

    // 初期ステートを決定
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

    // ヒープタイプに応じた初期ステートを決定
    if (heapType_ == D3D12_HEAP_TYPE_UPLOAD)
    {
        // アップロードヒープの場合、初期ステートはGENERIC_READにする
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }
    else if (heapType_ == D3D12_HEAP_TYPE_DEFAULT)
    {
        // デフォルトヒープの場合、初期ステートはCOMMONにする
        initialState = D3D12_RESOURCE_STATE_COMMON;
    }

    // バッファリソースを生成
    HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState, nullptr, IID_PPV_ARGS(&resource_));

    if (FAILED(hr))
    {
        throw std::runtime_error("巨大なバッファの生成に失敗しました。");
    }

    // 現在のリソースステートを初期ステートに設定
    currentState_ = initialState;

    // CPUからアクセス可能なUPLOADヒープの場合のみ永続マップを行う
    if (heapType_ == D3D12_HEAP_TYPE_UPLOAD)
    {
        D3D12_RANGE readRange{ 0, 0 }; // CPUからの読み込みは行わない
        resource_->Map(0, &readRange, &mappedData_);
    }
}

/// @brief デストラクタ
Detail::SubAllocatedBuffer::~SubAllocatedBuffer()
{
    if (resource_)
    {
        if (mappedData_)
        {
            resource_->Unmap(0, nullptr);
            mappedData_ = nullptr;
        }
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

    // GPUアクセス用アドレスは、リソースのGPU仮想アドレスにオフセットを加算して計算する
    region.gpuAddress = resource_->GetGPUVirtualAddress() + alloc.offset;

    // CPUアクセス用アドレスは、永続マップされたアドレスが存在する場合のみ計算する
    if (mappedData_)
    {
        region.cpuAddress = static_cast<uint8_t*>(mappedData_) + alloc.offset;
    }
    else
    {
        region.cpuAddress = nullptr;
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

/// @brief バリアを張る
/// @param commandList 
/// @param newState 
void Detail::SubAllocatedBuffer::Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState)
{
    // アップロードヒープの場合はバリアを張る必要がない
    if (heapType_ == D3D12_HEAP_TYPE_UPLOAD)
        return;

    // 現在のステートと異なる場合のみバリアを発行
    if (currentState_ != newState)
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource_.Get();
        barrier.Transition.StateBefore = currentState_;
        barrier.Transition.StateAfter = newState;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(1, &barrier);

        // ステートを更新
        currentState_ = newState;
    }
}
