#include "OffscreenResource.hpp"
#include <cassert>
#include "Func/ResourceFunc/ResourceFunc.hpp"
#include "RenderContext/BarrierBatcher/BarrierBatcher.hpp"
#include "Vector/Vector4/Vector4.hpp"

#include "Engine.hpp"

/// @brief デストラクタ
Detail::OffscreenResource::~OffscreenResource()
{
	if (heap_)
	{
		for (int i = 0; i < resource_.size(); ++i)
		{
			// RTVハンドルを解放する
			heap_->FreeRtvDescriptorHandle(rtvHandle_[i]);

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
/// @param heap 
/// @param allocator 
/// @param width
/// @param height
void Detail::OffscreenResource::Initialize(ID3D12Device* device, DXHeap* heap, PlacedResourceAllocator* allocator, int32_t width, int32_t height)
{
    assert(device);
    assert(heap);
    assert(allocator);

    auto engine = Engine::GetInstance();
    heap_ = heap;
    allocator_ = allocator;

    int maxBufferCount = engine->GetMaxBufferCount();
    if (maxBufferCount > 0) maxBufferCount_ = maxBufferCount;

    // リソース、RTVハンドル、SRVハンドル、配置リソースの割り当て情報をリサイズ
    resource_.resize(maxBufferCount_);
    rtvHandle_.resize(maxBufferCount_);
    srvHandle_.resize(maxBufferCount_);
    allocations_.resize(maxBufferCount_);

    // リソースステートの初期化
    InitializeStates(maxBufferCount_, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // リソース設定
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // RTV/SRVと同じフォーマット
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // RTVを許可

    // クリアカラーの最適化設定
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = desc.Format;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 0.0f;

    // サイズとアライメントの取得
    D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);

    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        // メモリ割り当てとリソース配置
        allocations_[i] = allocator_->Allocate(allocInfo.SizeInBytes, allocInfo.Alignment);

        // リソースの作成
        HRESULT hr = device->CreatePlacedResource(allocator_->GetHeap(allocations_[i].heapIndex),allocations_[i].offset,&desc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,&clearValue, IID_PPV_ARGS(&resource_[i]));

        if (FAILED(hr)) throw std::runtime_error("OffscreenResource: CreatePlacedResource に失敗しました");


        // RTVの設定
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;

        // ビューの生成
        rtvHandle_[i] = heap->GetRtvDescriptorHandle();
        device->CreateRenderTargetView(resource_[i].Get(), &rtvDesc, rtvHandle_[i]);
        if (engine) engine->Log(LogLevel::Info, "RTV生成");



        // SRVの設定
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;

        // ビューの生成
        srvHandle_[i] = heap->GetSrvDescriptorHandle();
        device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
        if (engine) engine->Log(LogLevel::Info, "SRV生成");
    }
}

/// @brief サイズを作り直す
/// @param device 
/// @param width
/// @param height
void Detail::OffscreenResource::Resize(ID3D12Device* device, int32_t width, int32_t height)
{
    if (width == 0 || height == 0) return;

    // リソース設定を新しいサイズで構築
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    // クリアカラーの最適化設定
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = desc.Format;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 0.0f;

    // サイズとアライメントの取得
    D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);

    for (uint32_t i = 0; i < maxBufferCount_; ++i)
    {
        // ステートを初期化する
        SetState(i, D3D12_RESOURCE_STATE_RENDER_TARGET);


        // 古いリソースの破棄とメモリの返却
        resource_[i].Reset();
        if (allocator_)allocator_->Free(allocations_[i]);

        // 新しいサイズでメモリを再確保
        allocations_[i] = allocator_->Allocate(allocInfo.SizeInBytes, allocInfo.Alignment);

        // 新しいPlaced Resourceの作成
        HRESULT hr = device->CreatePlacedResource(allocator_->GetHeap(allocations_[i].heapIndex), allocations_[i].offset, &desc,
            D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&resource_[i]));

        if (FAILED(hr)) throw std::runtime_error("OffscreenResource: リサイズ時の CreatePlacedResource に失敗");


        // RTVの設定
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = desc.Format;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        // ビューの生成
        device->CreateRenderTargetView(resource_[i].Get(), &rtvDesc, rtvHandle_[i]);


        // SRVの設定
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;

        // ビューの生成
        device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, srvHandle_[i].cpuHandle);
    }
}

/// @brief バリアを張る
/// @param commandList 
/// @param before 
/// @param after 
/// @param frameIndex
void Detail::OffscreenResource::Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex)
{
	TransitionBarrier(resource_[frameIndex].Get(), before, after, commandList);
}

/// @brief 全てのバリアを張る
/// @param commandList 
/// @param before 
/// @param after 
void Detail::OffscreenResource::AllBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	for (auto& res : resource_)
	{
		TransitionBarrier(res.Get(), before, after, commandList);
	}
}

/// @brief レンダーターゲットの設定とクリア
/// @param commandList 
/// @param dsvHandle 
/// @param frameIndex
void Detail::OffscreenResource::ClearRenderTarget(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, uint32_t frameIndex)
{
	// nullptrチェック
	assert(commandList);

	// 設定
	commandList->OMSetRenderTargets(1, &rtvHandle_[frameIndex], false, &dsvHandle);

	// クリア
	float clearColor[] = { 0.0f, 0.0f ,0.0f, 0.0f };
	commandList->ClearRenderTargetView(rtvHandle_[frameIndex], clearColor, 0, nullptr);
}

/// @brief レンダーターゲットの設定
/// @param commandList 
/// @param dsvHandle 
/// @param frameIndex
void Detail::OffscreenResource::SetRenderTarget(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, uint32_t frameIndex)
{
	// nullptrチェック
	assert(commandList);

	// 設定
	commandList->OMSetRenderTargets(1, &rtvHandle_[frameIndex], false, &dsvHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::OffscreenResource::RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	// nullptrチェック
	assert(commandList);

	// テクスチャ
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param rootParameterIndex 
/// @param frameIndex
void Detail::OffscreenResource::RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex)
{
	// nullptrチェック
	assert(commandList);

	// テクスチャ
	commandList->SetComputeRootDescriptorTable(rootParameterIndex, srvHandle_[frameIndex].gpuHandle);
}

/// @brief リソースを取得する
/// @param bufferIndex 
/// @return 
ID3D12Resource* Detail::OffscreenResource::Get(uint32_t bufferIndex) const
{
    // バッファインデックスが範囲内であることを確認する
    assert(bufferIndex < resource_.size());

    // リソースを返す
    return resource_[bufferIndex].Get();
}
