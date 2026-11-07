#pragma once
#include <vector>
#include <cassert>
#include <typeindex>

#include "Func/ResourceFunc/ResourceFunc.hpp"
#include "RenderContext/DXHeap/DXHeap.hpp"
#include "../IResource.hpp"

#include "Store/ResourceStore/BufferAllocator/BufferAllocator.hpp"

namespace Detail
{
	/// @brief 定数バッファリソース : インタフェース
    class IConstantBufferResource : public IResource
	{
	public:

		/// @brief コンストラクタ
		IConstantBufferResource() = default;

		/// @brief 仮想デストラクタ
		virtual ~IConstantBufferResource() = default;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex 
		virtual void RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex 
		virtual void RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

        /// @brief GPU仮想アドレスを取得する
        /// @param frameIndex 
        /// @return 
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t frameIndex) const { return regions_[frameIndex].gpuAddress; }

		/// @brief 型情報を取得する
		/// @return 
		virtual std::type_index GetTypeIndex() const = 0;


	protected:

        // バッファアロケータ
        BufferAllocator* allocator_ = nullptr;

        // サブアロケーションされたバッファ領域の情報配列
        std::vector<SubAllocatedRegion> regions_;

		/// @brief 最大バッファ数
		uint32_t maxBufferCount_ = 1;
	};

}
