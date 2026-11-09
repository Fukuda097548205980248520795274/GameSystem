#pragma once
#include "Store/ResourceStore/BufferAllocator/BufferAllocator.hpp"

namespace Detail
{
    class IResource
    {
    public:

        /// @brief コンストラクタ
        IResource() = default;

        /// @brief デストラクタ
        virtual ~IResource() = default;

        /// @brief リソースを取得する
        /// @param bufferIndex 
        /// @return 
        virtual ID3D12Resource* Get(uint32_t bufferIndex = 0) const = 0;

        /// @brief 現在のリソース状態を取得する
        /// @param bufferIndex 
        /// @return 
        D3D12_RESOURCE_STATES GetState(uint32_t bufferIndex = 0) const;

        /// @brief リソース状態を更新する
        /// @param bufferIndex 
        /// @param state 
        void SetState(uint32_t bufferIndex, D3D12_RESOURCE_STATES state) { if (bufferIndex < currentStates_.size()) currentStates_[bufferIndex] = state; }


    protected:

        /// @brief ステートの初期化
        /// @param bufferCount 
        /// @param initialState 
        void InitializeStates(uint32_t bufferCount, D3D12_RESOURCE_STATES initialState) { currentStates_.resize(bufferCount, initialState); }

        // 現在のリソースステート
        std::vector<D3D12_RESOURCE_STATES> currentStates_;
    };
}
