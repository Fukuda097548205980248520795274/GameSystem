#pragma once
#include <vector>
#include <cassert>


#include "Func/ResourceFunc/ResourceFunc.hpp"
#include "RenderContext/DXHeap/DXHeap.hpp"
#include "../IResource.hpp"

namespace Detail
{
	/// @brief ストラクチャードバッファリソース : インタフェーズ
    class IStructuredBufferResource : public IResource
	{
	public:

		/// @brief コンストラクタ
		IStructuredBufferResource() = default;

		/// @brief 仮想デストラクタ
		virtual ~IStructuredBufferResource() = default;

		/// @brief リサイズ
		/// @param device 
		/// @param num 
		virtual void Resize(BufferAllocator* allocator, ID3D12Device* device, UINT num) = 0;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex 
		void RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex 
		void RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);


	protected:

		/// @brief ヒープ
		DXHeap* heap_ = nullptr;

		/// @brief 最大バッファ数
		uint32_t maxBufferCount_ = 1;

        // バッファアロケータ
        BufferAllocator* allocator_ = nullptr;

        // サブアロケータから確保したメモリ領域
        std::vector<SubAllocatedRegion> regions_;

		/// @brief SRVハンドル
		std::vector<SRVDescriptorHandle> handle_;

        // リサイズ前の要素数を保持するメンバ変数
        int32_t oldSize_ = 0;
	};
}
