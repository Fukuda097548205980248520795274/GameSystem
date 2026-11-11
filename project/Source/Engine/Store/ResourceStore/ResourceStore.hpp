#pragma once
#include <memory>
#include <queue>
#include <vector>
#include <unordered_map>
#include "Resource/ConstantBufferResource/ConstantBufferResource.hpp"
#include "BufferAllocator/BufferAllocator.hpp"
#include "PlacedResourceAllocator/PlacedResourceAllocator.hpp"

namespace Detail
{
	/// @brief レンダーリソースストア
	class ResourceStore
	{
    public:

        /// @brief コンストラクタ
        /// @param device 
        ResourceStore(ID3D12Device* device){ Initialize(device); }

        /// @brief デストラクタ
        ~ResourceStore() = default;

        /// @brief バッファアロケータを取得する
        /// @return 
        BufferAllocator* GetBufferAllocator() const { return bufferAllocator_.get(); }

        /// @brief 配置済みアロケータを取得する
        /// @return 
        PlacedResourceAllocator* GetPlacedResourceAllocator() const { return placedAllocator_.get(); }

        /// @brief 任意のリソースを生成・登録する
        /// @tparam ResType 
        /// @tparam ...Args 
        /// @param ...args 
        /// @return 
        template<class ResType, class... Args>
        int32_t Create(Args&&... args)
        {
            // 型情報を取得
            auto typeIdx = std::type_index(typeid(ResType));
            auto& freeQueue = freeHandles_[typeIdx];

            // 空きハンドルが存在する場合は再利用
            if (!freeQueue.empty())
            {
                int32_t reusedHandle = freeQueue.front();
                freeQueue.pop();

                // リソースを再生成して登録
                resources_[reusedHandle].resource = std::make_unique<ResType>(std::forward<Args>(args)...);
                return reusedHandle;
            }

            // 新しいリソースを作成して登録
            resources_.push_back({ std::make_unique<ResType>(std::forward<Args>(args)...), typeIdx });
            return static_cast<int32_t>(resources_.size() - 1);
        }

        /// @brief ハンドルを指定してリソースを解放（空きキューに戻す）
        /// @param handle 
        void Release(int32_t handle);

        /// @brief 型を指定してリソースを取得する
        /// @tparam ResType 
        /// @param handle 
        /// @return 
        template<class ResType>
        ResType* Get(int32_t handle)
        {
            // ハンドルが有効で、リソースが存在する場合に取得
            if (handle >= 0 && handle < resources_.size() && resources_[handle].resource)
            {
                // 型が一致しない場合は nullptr を返す
                if (resources_[handle].type != std::type_index(typeid(ResType)))
                    return nullptr;

                // 型が一致する場合にキャストして返す
                return static_cast<ResType*>(resources_[handle].resource.get());
            }

            return nullptr;
        }


    private:

        /// @brief 初期化
        /// @param device 
        void Initialize(ID3D12Device* device);


        // リソースエントリ構造体
        struct ResourceEntry
        {
            std::unique_ptr<IResource> resource;
            std::type_index type;
        };

        // バッファアロケータ <- これはresourcesより先に消す
        std::unique_ptr<BufferAllocator> bufferAllocator_;

        // 配置済みリソースアロケータ
        std::unique_ptr<PlacedResourceAllocator> placedAllocator_;

        // リソースを一元管理
        std::vector<ResourceEntry> resources_;

        // 型情報に基づいて空きハンドルのキューを管理
        std::unordered_map<std::type_index, std::queue<int32_t>> freeHandles_;
	};
}
