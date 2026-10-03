#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

namespace Detail
{
	/// @brief トランジションバリアを張る
	/// @param resource 
	/// @param before 
	/// @param after 
	/// @param commandList 
	void TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, ID3D12GraphicsCommandList* commandList);

	/// @brief UAVバリアを張る
	/// @param resource 
	/// @param commandList 
	void UAVBarrier(ID3D12Resource* resource, ID3D12GraphicsCommandList* commandList);
}