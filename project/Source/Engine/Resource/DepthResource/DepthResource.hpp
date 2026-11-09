#pragma once
#include <utility>
#include <vector>

#include "RenderContext/DXHeap/DXHeap.hpp"
#include "Store/ResourceStore/PlacedResourceAllocator/PlacedResourceAllocator.hpp"
#include "../IResource.hpp"

namespace Detail
{
	/// @brief DXヒープ
	class DXHeap;

	/// @brief 深度リソース
    class DepthResource : public IResource
	{
	public:

		/// @brief 初期化
		/// @param device 
		/// @param width 
		/// @param height 
		/// @param heap 
		/// @param allocator 
		DepthResource(ID3D12Device* device, int32_t width, int32_t height, DXHeap* heap, PlacedResourceAllocator* allocator) { Initialize(device, width, height, heap, allocator); }

		/// @brief デストラクタ
		~DepthResource();

		/// @brief サイズを作り直す
		/// @param device 
		/// @param width 
		/// @param height 
		void Resize(ID3D12Device* device, int32_t width, int32_t height);

		/// @brief バリアを張る
		/// @param commandList 
		/// @param before 
		/// @param after 
		/// @param frameIndex
		void Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex);

		/// @brief デプスステンシルのクリア
		/// @param commandList 
		/// @param frameIndex
		void ClearDepthStencil(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);

		/// @brief レンダーターゲットに設定する
		/// @param commandList 
		/// @param frameIndex 
		void SetRenderTargetView(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);

		/// @brief DSV用ハンドルを取得する
		/// @param frameIndex
		/// @return 
		D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle(uint32_t frameIndex) { return dsvHandle_[frameIndex]; }

        /// @brief リソースを取得する
        /// @param bufferIndex 
        /// @return 
        ID3D12Resource* Get(uint32_t bufferIndex = 0) const override;

		/// @brief コマンドリストに登録する
		/// @param commandList
		/// @param rootParameterIndex
		/// @param frameIndex
		void RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief SRVをコマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex
		void RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief DSV用ハンドル（読み取り専用）を取得する
		/// @param frameIndex
		/// @return 
		D3D12_CPU_DESCRIPTOR_HANDLE GetDsvReadOnlyHandle(uint32_t frameIndex) { return dsvReadOnlyHandle_[frameIndex]; }


	private:

		/// @brief 初期化
		/// @param device 
		/// @param width 
		/// @param height 
		/// @param heap 
		/// @param allocator 
		void Initialize(ID3D12Device* device, int32_t width, int32_t height, DXHeap* heap, PlacedResourceAllocator* allocator);

		/// @brief ヒープ
		DXHeap* heap_ = nullptr;

		// 最大バッファ数
		uint32_t maxBufferCount_ = 1;

		// リソース
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;

		/// @brief SRV用ハンドル
		std::vector<SRVDescriptorHandle> srvHandle_;

		// DSV用ハンドル
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dsvHandle_;

		/// @brief DSV用ハンドル（読み取り専用）
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dsvReadOnlyHandle_;
        
        /// @brief 配置リソースアロケータ
        PlacedResourceAllocator* allocator_ = nullptr;

        // 配置リソースの割り当て情報
        std::vector<PlacedAllocation> allocations_;
	};
}
