#include "BarrierBatcher.hpp"
#include <cassert>

/// @brief トランジションバリアを追加する
/// @param resource 
/// @param before 
/// @param after 
/// @param subresource 
void Detail::BarrierBatcher::AddTransition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, UINT subresource)
{
    assert(resource);

    // 状態が変化しない場合はバリアを張らない
    if (before == after) return;

    // バリアの設定
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = subresource;

    // バリアを蓄積する
    barriers_.push_back(barrier);
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
