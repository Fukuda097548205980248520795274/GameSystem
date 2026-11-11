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
        [[nodiscard]]
        int32_t Create(Args&&... args)
        {
            // リソースを生成
            auto resource = std::make_unique<ResType>(std::forward<Args>(args)...);
            auto typeIdx = std::type_index(typeid(ResType));

            // 空きハンドルが存在する場合は再利用
            if (!freeHandles_.empty())
            {
                int32_t reusedHandle = freeHandles_.front();
                freeHandles_.pop();

                // リソースと型情報を上書き
                resources_[reusedHandle] = { std::move(resource), typeIdx };
                return reusedHandle;
            }

            // 新しいリソースを作成して登録
            resources_.push_back({ std::move(resource), typeIdx });
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
        [[nodiscard]] ResType* Get(int32_t handle) const
        {
            // ハンドルが有効かチェック
            if (!IsValid(handle))return nullptr;

            // リソースエントリを取得
            const auto& entry = resources_[handle];
            if (entry.type != std::type_index(typeid(ResType)))return nullptr;

            // 型が一致する場合はリソースを返す
            return static_cast<ResType*>(entry.resource.get());
        }


    private:

        // コピー禁止
        ResourceStore(const ResourceStore&) = delete;
        ResourceStore& operator=(const ResourceStore&) = delete;
        ResourceStore(ResourceStore&&) = delete;
        ResourceStore& operator=(ResourceStore&&) = delete;

        /// @brief ハンドルが有効かチェックする
        /// @param handle 
        /// @return 
        [[nodiscard]]
        bool IsValid(int32_t handle) const noexcept
        {
            return handle >= 0 && handle < resources_.size() && resources_[handle].resource != nullptr;
        }

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
        std::queue<int32_t> freeHandles_;
	};
}
