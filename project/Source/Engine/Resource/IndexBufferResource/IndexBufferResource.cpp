#include "IndexBufferResource.h"
#include "Func/ResourceFunc/ResourceFunc.h"
#include <cassert>

/// @brief 初期化
/// @param device 
/// @param numIndex 
/// @param maxBufferCount 
void Detail::IndexBufferResource::Initialize(ID3D12Device* device, uint32_t numIndex, uint32_t maxBufferCount)
{
	assert(device);
	assert(numIndex > 0);

	// バッファ数を設定する
	maxBufferCount_ = maxBufferCount_ > 1 ? static_cast<int32_t>(maxBufferCount_) : 1;

	// データ、リソース、ビューの配列を確保する
	data_.resize(maxBufferCount_);
	resource_.resize(maxBufferCount_);
	view_.resize(maxBufferCount_);

	for (int i = 0; i < maxBufferCount_; ++i)
	{
		// リソースを生成する
		resource_[i] = CreateBufferResource(device, sizeof(uint32_t) * numIndex);

		// ビューの設定
		view_[i].BufferLocation = resource_[i]->GetGPUVirtualAddress();
		view_[i].Format = DXGI_FORMAT_R32_UINT;
		view_[i].SizeInBytes = sizeof(uint32_t) * numIndex;

		// データを割り当てる
		resource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&data_[i]));
	}
}

/// @brief コマンドリストに登録する
/// @param commandList 
/// @param frameIndex
void Detail::IndexBufferResource::Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	assert(commandList);

	// フレームインデックスを最大バッファ数でラップする
	frameIndex = static_cast<int32_t>(frameIndex) < maxBufferCount_ ? frameIndex : 0;

	// インデックスバッファを設定する
	commandList->IASetIndexBuffer(&view_[frameIndex]);
}