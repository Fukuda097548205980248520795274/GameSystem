#pragma once
#include <memory>
#include <queue>
#include <vector>
#include <unordered_map>
#include "Resource/ConstantBufferResource/ConstantBufferResource.h"

namespace Detail
{
	/// @brief レンダーリソースストア
	class RenderResourceStore
	{
    public:

		/// @brief 定数バッファを生成する
        /// @tparam T 
        /// @param device 
        /// @param  
        /// @param maxBufferCount 
        /// @return 
        template<class T>
        int32_t CreateConstantBuffer(ID3D12Device* device, int32_t maxBufferCount)
        {
			// 型情報に基づいて空きハンドルのキューを取得
            auto& freeQueue = freeHandles_[typeid(T)];

            // 空きがあればリソースを再生成せずにハンドルだけ返す
            if (!freeQueue.empty())
            {
                int32_t reusedHandle = freeQueue.front();
                freeQueue.pop();
                return reusedHandle;
            }

            // 空きがなければ新規作成して末尾に追加
            constantBuffers_.push_back(std::make_unique<Detail::ConstantBufferResource<T>>(device, maxBufferCount));
            return static_cast<int32_t>(constantBuffers_.size() - 1);
        }

		/// @brief 定数バッファを更新する
        /// @tparam T 
        /// @param cbHandle 
        /// @param data 
        /// @param frameIndex 
        template<class T>
        void UpdateConstantBuffer(int32_t cbHandle, const T& data, uint32_t frameIndex)
        {
            if (cbHandle >= 0 && cbHandle < constantBuffers_.size() && constantBuffers_[cbHandle])
            {
                // IConstantBufferResource からキャストして GetData を呼び出す
                auto* cb = static_cast<Detail::ConstantBufferResource<T>*>(constantBuffers_[cbHandle].get());
                T* mappedData = cb->GetData(frameIndex);
                if (mappedData)
                {
                    *mappedData = data;
                }
            }
        }

		/// @brief 定数バッファのGPU仮想アドレスを取得する
        /// @param cbHandle 
        /// @param frameIndex 
        /// @return 
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(int32_t cbHandle, uint32_t frameIndex);

		/// @brief 定数バッファを解放する
        /// @param cbHandle 
        void ReleaseConstantBuffer(int32_t cbHandle);

		/// @brief 定数バッファを取得する
		/// @param cbHandle 
		/// @return 
        IConstantBufferResource* GetConstantBuffer(int32_t cbHandle);


    private:

        // 定数バッファを一元管理
        std::vector<std::unique_ptr<IConstantBufferResource>> constantBuffers_;

		// 型情報に基づいて空きハンドルのキューを管理
        std::unordered_map<std::type_index, std::queue<int32_t>> freeHandles_;
	};
}