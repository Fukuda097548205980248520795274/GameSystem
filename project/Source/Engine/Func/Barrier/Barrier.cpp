#include "Barrier.h"
#include <cassert>

/// @brief トランジションバリアを張る
/// @param resource 
/// @param before 
/// @param after 
/// @param commandList 
void Detail::TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, ID3D12GraphicsCommandList* commandList)
{
	// nullptrチェック
	assert(resource);
	assert(commandList);

	// バリアの設定
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// バリアを張る
	commandList->ResourceBarrier(1, &barrier);
}

/// @brief UAVバリアを張る
/// @param resource 
/// @param commandList 
void Detail::UAVBarrier(ID3D12Resource* resource, ID3D12GraphicsCommandList* commandList)
{
	// nullptrチェック
	assert(resource);
	assert(commandList);

	// バリアの設定
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = resource;

	// バリアを張る
	commandList->ResourceBarrier(1, &barrier);
}