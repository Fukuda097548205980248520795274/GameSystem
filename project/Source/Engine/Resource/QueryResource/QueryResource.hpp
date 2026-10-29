#pragma once
#include "IQueryResource.hpp"
#include "RenderContext/DXHeap/DXHeap.hpp"

namespace Detail
{
	/// @brief クエリリソース
	/// @tparam T 
	template<typename T>
	class QueryResource : public IQueryResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param heap 
		/// @param maxBufferCount 
		QueryResource(ID3D12Device* device, DXHeap* heap, uint32_t maxBufferCount) { Initialize(device, heap, maxBufferCount); }

		/// @brief デストラクタ
		~QueryResource();

		/// @brief 結果を取得する
		/// @param frameIndex 
		/// @return タイムスタンプの値
		T GetResult(uint32_t frameIndex);


	private:

		/// @brief 初期化
		/// @param device 
		/// @param heap 
		/// @param maxBufferCount 
		void Initialize(ID3D12Device* device, DXHeap* heap, uint32_t maxBufferCount = 1);
	};
}


/// @brief デストラクタ
template<typename T>
Detail::QueryResource<T>::~QueryResource()	
{
	// クエリインデックスをヒープに返却する
	if (heap_)
	{
		heap_->FreeQueryIndex(queryIndex_);
	}
}

/// @brief 結果を取得する
/// @param frameIndex 
/// @return 
template<typename T>
T Detail::QueryResource<T>::GetResult(uint32_t frameIndex)
{
	assert(frameIndex < maxBufferCount_);

	T result = 0;
	void* mappedData = nullptr;

	// リードバックバッファをマップしてデータを読み取る
	D3D12_RANGE readRange{ 0, sizeof(T) };
	HRESULT hr = readbackBuffers_[frameIndex]->Map(0, &readRange, &mappedData);
	if (SUCCEEDED(hr))
	{
		result = *reinterpret_cast<T*>(mappedData);

		// 読み取り完了後にアンマップする
		D3D12_RANGE emptyRange{ 0, 0 };
		readbackBuffers_[frameIndex]->Unmap(0, &emptyRange);
	}

	return result;
}


/// @brief 初期化
/// @param device 
/// @param heap 
/// @param maxBuffer 
template<typename T>
void Detail::QueryResource<T>::Initialize(ID3D12Device* device, DXHeap* heap, uint32_t maxBuffer)
{
	// nullptrチェック
	assert(device);
	assert(heap);

	heap_ = heap;

	// 最大バッファ数を設定
	if (maxBuffer > maxBufferCount_) maxBufferCount_ = maxBuffer;

	// DXHeapから空いているクエリインデックスを取得
	queryIndex_ = heap_->GetQueryIndex();

	// リードバックバッファの配列をリサイズ
	readbackBuffers_.resize(maxBufferCount_);

	// ヒーププロパティの設定 (Readback用)
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	// リソースの設定
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Alignment = 0;
	resDesc.Width = sizeof(T);
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// フレームバッファ（ダブルバッファ）分だけReadbackリソースを生成
	for (uint32_t i = 0; i < maxBufferCount_; ++i)
	{
		HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&readbackBuffers_[i])
		);

		if (!SUCCEEDED(hr))
		{
			throw std::runtime_error("クエリ用リードバックリソースの生成に失敗しました");
		}
	}
}