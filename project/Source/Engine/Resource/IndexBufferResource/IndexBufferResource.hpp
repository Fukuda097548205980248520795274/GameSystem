#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <vector>
#include <wrl.h>
#include "../IResource.hpp"

namespace Detail
{
	/// @brief インデックスバッファリソース
    class IndexBufferResource : public IResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param numIndex 
		/// @param maxBufferCount 
		IndexBufferResource(BufferAllocator* allocator, ID3D12Device* device, uint32_t numIndex, uint32_t maxBufferCount) { Initialize(allocator, device, numIndex, maxBufferCount); }

		/// @brief デストラクタ
		~IndexBufferResource();

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param frameIndex
		void Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);

		/// @brief データを取得する
		/// @param frameIndex 
		/// @return 
		uint32_t* GetData(uint32_t frameIndex) { return static_cast<uint32_t*>(regions_[frameIndex].cpuAddress); }

		/// @brief インデックス数を取得する
		/// @return 
		UINT GetNumIndex() const { return numIndex_; }

        /// @brief リソースを取得する
        /// @param bufferIndex 
        /// @return 
        ID3D12Resource* Get(uint32_t bufferIndex = 0) const override;


		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

	private:

		/// @brief 初期化
		/// @param device 
		/// @param numIndex 
		/// @param maxBufferCount 
		void Initialize(BufferAllocator* allocator, ID3D12Device* device, uint32_t numIndex, uint32_t maxBufferCount);

		/// @brief 最大バッファ数
		int32_t maxBufferCount_ = 1;

		/// @brief インデックス数
		UINT numIndex_ = 0;

		/// @brief ビュー
		std::vector<D3D12_INDEX_BUFFER_VIEW> view_;

        // @brief バッファアロケータ
        BufferAllocator* allocator_ = nullptr;

        // @brief サブアロケートされた領域
        std::vector<SubAllocatedRegion> regions_;
	};
}
