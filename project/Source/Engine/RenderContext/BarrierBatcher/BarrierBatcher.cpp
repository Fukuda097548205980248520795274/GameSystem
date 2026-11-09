#include "BarrierBatcher.hpp"
#include <cassert>

#include "Resource/IResource.hpp"

/// @brief トランジションバリアを追加する
/// @param resource 
/// @param before 
/// @param after 
/// @param subresource 
void Detail::BarrierBatcher::AddTransition(Detail::IResource* resource, uint32_t bufferIndex, D3D12_RESOURCE_STATES newState, UINT subresource)
{
    assert(resource);

    // 対象インデックスの生のID3D12Resourceを取得
    ID3D12Resource* rawResource = resource->Get(bufferIndex);
    assert(rawResource);

    // リソースの現在の状態を取得
    D3D12_RESOURCE_STATES currentState = resource->GetState(bufferIndex);

    // 状態が異なる場合のみバリアを張る
    if (currentState != newState)
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = rawResource;
        barrier.Transition.StateBefore = currentState;
        barrier.Transition.StateAfter = newState;
        barrier.Transition.Subresource = subresource;

        barriers_.push_back(barrier);

        // リソース側の内部状態を新しい状態に更新
        resource->SetState(bufferIndex, newState);
    }
}

/// @brief UAVバリアを追加する
/// @param resource 
void Detail::BarrierBatcher::AddUAV(ID3D12Resource* resource)
{
    assert(resource);

    // バリアの設定
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource;

    // バリアを蓄積する
    barriers_.push_back(barrier);
}

/// @brief 蓄積されたバリアを一括でコマンドリストに積む
void Detail::BarrierBatcher::Flush()
{
    if (!barriers_.empty())
    {
        // バリアをコマンドリストに積む
        commandList_->ResourceBarrier(static_cast<UINT>(barriers_.size()), barriers_.data());
        barriers_.clear();
    }
}

/// @brief トランジションバリア
/// @param resource 
/// @param before 
/// @param after 
/// @param commandList 
void Detail::TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, ID3D12GraphicsCommandList* commandList)
{
    assert(resource);
    assert(commandList);

    // 状態が変化しない場合はバリアを張らない
    if (before == after) return;

    // バリアの設定
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    // バリアを張る
    commandList->ResourceBarrier(1, &barrier);
}

/// @brief UAVバリア
/// @param resource 
/// @param commandList 
void Detail::UAVBarrier(ID3D12Resource* resource, ID3D12GraphicsCommandList* commandList)
{
    assert(resource);
    assert(commandList);

    // バリアの設定
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource;

    // バリアを張る
    commandList->ResourceBarrier(1, &barrier);
}
