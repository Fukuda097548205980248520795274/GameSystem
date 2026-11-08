#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

namespace Detail
{
    class BarrierBatcher
    {
    public:

        /// @brief コンストラクタ
        /// @param commandList 
        explicit BarrierBatcher(ID3D12GraphicsCommandList* commandList) : commandList_(commandList) {}

        /// @brief トランジションバリアを追加する
        /// @param resource 
        /// @param before 
        /// @param after 
        /// @param subresource 
        void AddTransition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

        /// @brief UAVバリアを追加する
        /// @param resource 
        void AddUAV(ID3D12Resource* resource);

        /// @brief 蓄積されたバリアを一括でコマンドリストに積む
        void Flush();

        /// @brief 蓄積されているバリアの数を取得する
        /// @return 
        size_t GetCount() const { return barriers_.size(); }

    private:

        // コマンドリストへのポインタ
        ID3D12GraphicsCommandList* commandList_ = nullptr;

        // 蓄積されたバリアのリスト
        std::vector<D3D12_RESOURCE_BARRIER> barriers_;
    };

    /// @brief トランジションバリア
    /// @param resource 
    /// @param before 
    /// @param after 
    /// @param commandList 
    void TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, ID3D12GraphicsCommandList* commandList);

    /// @brief UAVバリア
    /// @param resource 
    /// @param commandList 
    void UAVBarrier(ID3D12Resource* resource, ID3D12GraphicsCommandList* commandList);
}
