#pragma once
#include "IStructuredBufferResource.h"

namespace Detail
{
	/// @brief 構造化バッファリソース
	/// @tparam T 
	template<typename T>
	class StructuredBufferResource : public IStructuredBufferResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param heap 
		/// @param num 
		/// @param frameIndex 
		StructuredBufferResource(ID3D12Device* device, DXHeap* heap, UINT num, uint32_t frameIndex) { Initialize(device, heap, num, frameIndex); }

		/// @brief デストラクタ
		~StructuredBufferResource();

		/// @brief リサイズ
		/// @param device 
		/// @param num 
		void Resize(ID3D12Device* device, UINT num) override;

		/// @brief データを取得する
		/// @param frameIndex 
		/// @return 
		T* GetData(uint32_t frameIndex) { return data_[frameIndex]; }


	private:

		/// @brief 初期化
		/// @param device 
		/// @param heap 
		/// @param num 
		/// @param frameIndex
		void Initialize(ID3D12Device* device, DXHeap* heap, UINT num, uint32_t frameIndex);

		/// @brief データ
		std::vector<T*> data_;
	};
}

/// @brief デストラクタ
template <typename T>
Detail::StructuredBufferResource<T>::~StructuredBufferResource()
{
	if (heap_)
	{
		for (int i = 0; i < resource_.size(); ++i)
		{
			// SRVハンドルを解放する
			heap_->FreeSrvDescriptorHandle(handle_[i]);
		}
	}
}

/// @brief 初期化
/// @param device 
/// @param heap 
/// @param num 
/// @param log 
/// @param frameIndex
template <typename T>
void Detail::StructuredBufferResource<T>::Initialize(ID3D12Device* device, DXHeap* heap, UINT num, uint32_t frameIndex)
{
	// nullptrチェック
	assert(device);
	assert(heap);
	assert(num > 0);

	// ヒープを保存する
	heap_ = heap;

	// 最大バッファ数を取得する
	if (frameIndex > maxBufferCount_) maxBufferCount_ = frameIndex;
	
	// リソースとデータの配列をリサイズする
	resource_.resize(maxBufferCount_);
	data_.resize(maxBufferCount_);
	handle_.resize(maxBufferCount_);

	for (uint32_t i = 0; i < maxBufferCount_; ++i)
	{
		// リソース作成
		resource_[i] = CreateBufferResource(device, sizeof(T) * num);

		// データを割りあてる
		resource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&data_[i]));

		// SRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = num;
		srvDesc.Buffer.StructureByteStride = sizeof(T);

		// ハンドルを取得する
		handle_[i] = heap->GetSrvDescriptorHandle();

		// ビューの生成
		device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, handle_[i].cpuHandle);
	}
}

/// @brief リサイズ
/// @param device 
/// @param num 
template<typename T>
void Detail::StructuredBufferResource<T>::Resize(ID3D12Device* device, UINT num)
{
	// nullptrチェック
	assert(device);
	assert(num > 0);

	for (uint32_t i = 0; i < maxBufferCount_; ++i)
	{
		// 既存のリソースを解放する
		resource_[i].Reset();

		// リソース作成
		resource_[i] = CreateBufferResource(device, sizeof(T) * num);

		// データを割りあてる
		resource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&data_[i]));

		// SRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = num;
		srvDesc.Buffer.StructureByteStride = sizeof(T);

		// ビューの生成
		device->CreateShaderResourceView(resource_[i].Get(), &srvDesc, handle_[i].cpuHandle);
	}
}