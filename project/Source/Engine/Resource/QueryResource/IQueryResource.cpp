#include "IQueryResource.hpp"
#include <cassert>
#include "RenderContext/DXHeap/DXHeap.hpp"

/// @brief クエリを記録する（タイムスタンプを取得）
/// @param commandList 
void Detail::IQueryResource::Record(ID3D12GraphicsCommandList* commandList)
{
	assert(commandList);
	assert(heap_);

	// タイムスタンプクエリを記録する
	commandList->EndQuery(heap_->GetQueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex_);
}

/// @brief クエリ結果を解決（リードバックバッファへコピー）する
/// @param commandList 
/// @param frameIndex 
void Detail::IQueryResource::Resolve(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex)
{
	assert(commandList);
	assert(heap_);
	assert(frameIndex < maxBufferCount_);

	// クエリヒープからリードバックバッファへデータを解決(コピー)する
	commandList->ResolveQueryData(heap_->GetQueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex_, 1, readbackBuffers_[frameIndex].Get(), 0);
}