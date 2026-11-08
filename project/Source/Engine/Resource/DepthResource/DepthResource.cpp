#include "DepthResource.hpp"

#include "Engine.hpp"
#include "Func/Barrier/Barrier.hpp"
#include "Func/ResourceFunc/ResourceFunc.hpp"

/// @brief デストラクタ
Detail::DepthResource::~DepthResource()
{
	if (heap_)
	{
		for (int i = 0; i < resource_.size(); ++i)
		{
			// DSVハンドルを解放する
			heap_->FreeDsvDescriptorHandle(dsvHandle_[i]);

			// 読み取り専用のDSVハンドルを解放する
			heap_->FreeDsvDescriptorHandle(dsvReadOnlyHandle_[i]);

			// SRVハンドルを解放する
			heap_->FreeSrvDescriptorHandle(srvHandle_[i]);
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
/// @param width 
/// @param height 
/// @param heap 
/// @param log 
void Detail::DepthResource::Initialize(ID3D12Device* device, int32_t width, int32_t height, DXHeap* heap, PlacedResourceAllocator* allocator)
{
    assert(device);
    assert(heap);
    assert(allocator);

    heap_ = heap;
    allocator_ = allocator;

    auto engine = Engine::GetInstance();
    uint32_t maxBufferCount = engine->GetMaxBufferCount();
    if (maxBufferCount > 0) maxBufferCount_ = maxBufferCount;

    // リソース、DSVハンドル、SRVハンドル、配置リソースの割り当て情報をリサイズ
    resource_.resize(maxBufferCount_);
    dsvHandle_.resize(maxBufferCount_);
    dsvReadOnlyHandle_.resize(maxBufferCount_);
    srvHandle_.resize(maxBufferCount_);
    allocations_.resize(maxBufferCount_);

    // リソースの設定 (DSVとSRVで共有するためTYPELESSを指定)
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R24G8_TYPELESS; // リソース本体はTYPELESS
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 深度ステンシルを許可

    // 最適化されたクリア値
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // メモリサイズとアライメントをGPUに問い合わせる
    D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);

    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        // メモリ領域の確保
        allocations_[i] = allocator_->Allocate(allocInfo.SizeInBytes, allocInfo.Alignment);

        // PlacedResource の生成
        HRESULT hr = device->CreatePlacedResource(allocator_->GetHeap(allocations_[i].heapIndex),allocations_[i].offset,&desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,&clearValue,IID_PPV_ARGS(&resource_[i]));

        if (FAILED(hr)) throw std::runtime_error("DepthResource: CreatePlacedResource に失敗");

        // DSVの生成
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        // DSVの生成
        dsvHandle_[i] = heap->GetDsvDescriptorHandle();
        device->CreateDepthStencilView(resource_[i].Get(), &dsvDesc, dsvHandle_[i]);

        // 読み取り専用のDSVを生成する
        dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
        dsvReadOnlyHandle_[i] = heap->GetDsvDescriptorHandle();
        device->CreateDepthStencilView(resource_[i].Get(), &dsvDesc, dsvReadOnlyHandle_[i]);


        // SRVの設定
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        // ビューの生成
        srvHandle_[i] = heap->GetSrvDescriptorHandle();
        device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
    }
}

/// @brief サイズを作り直す
/// @param device 
/// @param width 
/// @param height 
void Detail::DepthResource::Resize(ID3D12Device* device, int32_t width, int32_t height)
{
    assert(device);

    // リソースの設定 (DSVとSRVで共有するためTYPELESSを指定)
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // 最適化されたクリア値
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // メモリサイズとアライメントをGPUに問い合わせる
    D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);

    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        // 既存のリソースとメモリを解放
        resource_[i].Reset();
        if (allocator_) allocator_->Free(allocations_[i]);

        // 新たなサイズでメモリ再確保
        allocations_[i] = allocator_->Allocate(allocInfo.SizeInBytes, allocInfo.Alignment);

        // PlacedResource の作成
        HRESULT hr = device->CreatePlacedResource(allocator_->GetHeap(allocations_[i].heapIndex), allocations_[i].offset, &desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&resource_[i]));

        if (FAILED(hr)) throw std::runtime_error("DepthResource: リサイズ時の CreatePlacedResource に失敗");

        // DSV 再生成
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(resource_[i].Get(), &dsvDesc, dsvHandle_[i]);

        // 読み取り専用DSV 再生成
        dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
        device->CreateDepthStencilView(resource_[i].Get(), &dsvDesc, dsvReadOnlyHandle_[i]);

        // SRV 再生成
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
    }
}

/// @brief バリアを張る
/// @param commandList 
/// @param before 
/// @param after 
/// @param frameIndex
void Detail::DepthResource::Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex)
{
	TransitionBarrier(resource_[frameIndex].Get(), before, after, commandList);
}

/// @brief デプスステンシルのクリア
/// @param commandList 
/// @param frameIndex
void Detail::DepthResource::ClearDepthStencil(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	// クリア
	commandList->ClearDepthStencilView(dsvHandle_[frameIndex], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

/// @brief レンダーターゲットに設定する
/// @param commandList 
/// @param frameIndex 
void Detail::DepthResource::SetRenderTargetView(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle_[frameIndex]);
}

/// @brief コマンドリストに登録する
/// @param commandList
/// @param rootParameterIndex
/// @param frameIndex
void Detail::DepthResource::RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	assert(commandList);
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief SRVをコマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::DepthResource::RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	assert(commandList);
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}
