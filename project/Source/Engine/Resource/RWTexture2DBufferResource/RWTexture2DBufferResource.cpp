#include "RWTexture2DBufferResource.hpp"
#include <cassert>

#include "RenderContext/BarrierBatcher/BarrierBatcher.hpp"
#include "Engine.hpp"

Detail::RWTexture2DBufferResource::~RWTexture2DBufferResource()
{
    // ヒープのハンドルを解放する
    if (heap_)
    {
        for (size_t i = 0; i < resource_.size(); ++i)
        {
            heap_->FreeSrvDescriptorHandle(srvHandle_[i]);
            heap_->FreeSrvDescriptorHandle(uavHandle_[i]);
        }
    }

    // PlacedResource用のメモリをアロケータに返却
    if (allocator_)
    {
        for (auto& alloc : allocations_)
        {
            allocator_->Free(alloc);
        }
    }
}

/// @brief 初期化
/// @param device 
/// @param commandList 
/// @param heap 
/// @param width 
/// @param height 
/// @param maxBufferCount 
void Detail::RWTexture2DBufferResource::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXHeap* heap, PlacedResourceAllocator* allocator,
    uint32_t width, uint32_t height, uint32_t maxBufferCount)
{
    assert(device);
    assert(heap);
    assert(allocator);

    width_ = width;
    height_ = height;
    heap_ = heap;
    allocator_ = allocator;

    if (maxBufferCount > 0) maxBufferCount_ = maxBufferCount;

    // リソース、SRVハンドル、UAVハンドル、配置リソースの割り当て情報をリサイズ
    resource_.resize(maxBufferCount_);
    srvHandle_.resize(maxBufferCount_);
    uavHandle_.resize(maxBufferCount_);
    allocations_.resize(maxBufferCount_);

    // リソースステートの初期化
    InitializeStates(maxBufferCount_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // テクスチャのリソース設定を定義
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0; // デバイスに自動決定させる
    desc.Width = width_;
    desc.Height = height_;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // テクスチャ専用レイアウト
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    // 必要なメモリサイズとアライメントをGPUに問い合わせる
    D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);

    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        // アロケータからメモリを確保（取得したサイズとアライメントを使用）
        allocations_[i] = allocator_->Allocate(allocInfo.SizeInBytes, allocInfo.Alignment);

        // 指定したヒープ領域にテクスチャリソースを配置
        HRESULT hr = device->CreatePlacedResource(allocator_->GetHeap(allocations_[i].heapIndex), allocations_[i].offset, &desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resource_[i]));

        if (FAILED(hr))
        {
            throw std::runtime_error("RWTexture2DBufferResource : CreatePlacedResource に失敗しました");
        }

        // UAVの生成
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = desc.Format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;

        // ビューの生成
        uavHandle_[i] = heap->GetSrvDescriptorHandle();
        device->CreateUnorderedAccessView(resource_[i].Get(), nullptr, &uavDesc, uavHandle_[i].cpuHandle);


        // SRVの生成
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        // ビューの生成
        srvHandle_[i] = heap->GetSrvDescriptorHandle();
        device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
    }
}

/// @brief リサイズ
/// @param device 
/// @param commandList 
/// @param width 
/// @param height 
void Detail::RWTexture2DBufferResource::Resize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height)
{
    assert(device);
    width_ = width;
    height_ = height;

    // テクスチャのリソース設定
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width_;
    desc.Height = height_;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);

    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        // 既存のリソースとメモリを解放
        resource_[i].Reset();
        if (allocator_) allocator_->Free(allocations_[i]);

        // 新しいサイズでメモリ再確保
        allocations_[i] = allocator_->Allocate(allocInfo.SizeInBytes, allocInfo.Alignment);

        // PlacedResource の作成
        HRESULT hr = device->CreatePlacedResource(allocator_->GetHeap(allocations_[i].heapIndex), allocations_[i].offset, &desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resource_[i]));

        if (FAILED(hr)) throw std::runtime_error("RWTexture2DBufferResource: リサイズ時の CreatePlacedResource に失敗");

        // UAVの再設定
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = desc.Format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        device->CreateUnorderedAccessView(resource_[i].Get(), nullptr, &uavDesc, uavHandle_[i].cpuHandle);

        // SRVの再設定
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
    }
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::RWTexture2DBufferResource::RegisterGraphicsSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::RWTexture2DBufferResource::RegisterGraphicsUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, uavHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::RWTexture2DBufferResource::RegisterComputeUAV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, uavHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
void Detail::RWTexture2DBufferResource::RegisterComputeSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief バリアを張る
/// @param commandList 
/// @param before 
/// @param after 
void Detail::RWTexture2DBufferResource::Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex)
{
	TransitionBarrier(resource_[frameIndex].Get(), before, after, commandList);
}

/// @brief 全てのバリアを張る
/// @param commandList 
/// @param before 
/// @param after 
void Detail::RWTexture2DBufferResource::AllBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	for (int i = 0; i < resource_.size(); ++i)
	{
		TransitionBarrier(resource_[i].Get(), before, after, commandList);
	}
}

/// @brief UAVバリアを張る
/// @param commandList 
/// @param frameIndex 
void Detail::RWTexture2DBufferResource::BarrierUAV(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	UAVBarrier(resource_[frameIndex].Get(), commandList);
}

/// @brief 全てのUAVバリアを張る
/// @param commandList 
void Detail::RWTexture2DBufferResource::AllBarrierUAV(ID3D12GraphicsCommandList* commandList)
{
	for (int i = 0; i < resource_.size(); ++i)
	{
		UAVBarrier(resource_[i].Get(), commandList);
	}
}

/// @brief リソースを取得する
/// @param bufferIndex 
/// @return 
ID3D12Resource* Detail::RWTexture2DBufferResource::Get(uint32_t bufferIndex) const
{
    // バッファインデックスが範囲内であることを確認する
    assert(bufferIndex < resource_.size());

    // リソースを返す
    return resource_[bufferIndex].Get();
}
