#pragma once
#include <cassert>
#include <vector>
#include "Func/ResourceFunc/ResourceFunc.hpp"
#include "../IResource.hpp"

namespace Detail
{
	/// @brief 頂点バッファリソース : インタフェース
    class IVertexBufferResource : public IResource
	{
	public:

		/// @brief コンストラクタ
		IVertexBufferResource() = default;

		/// @brief 仮想デストラクタ
		virtual ~IVertexBufferResource() = default;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param frameIndex 
		void Register(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);

		/// @brief 頂点数を取得する
		/// @return 
		UINT GetNumVertex() const { return numVertex_; }

        /// @brief リソースを取得する
        /// @param bufferIndex 
        /// @return 
        ID3D12Resource* Get(uint32_t bufferIndex = 0) const override;

        /// @brief サブアロケートされた領域を取得する
        /// @param bufferIndex 
        /// @return 
        const SubAllocatedRegion& GetRegion(uint32_t bufferIndex = 0) const;

		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;


	protected:

		/// @brief 最大バッファ数
		uint32_t maxBufferCount_ = 1;

		/// @brief 頂点数
		UINT numVertex_ = 0;

		/// @brief ビュー
		std::vector<D3D12_VERTEX_BUFFER_VIEW> view_;

        // @brief バッファアロケータ
        BufferAllocator* allocator_ = nullptr;

        // @brief サブアロケートされた領域
        std::vector<SubAllocatedRegion> regions_;
	};
}
