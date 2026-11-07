#pragma once
#include <memory>
#include <queue>
#include <vector>
#include <unordered_map>
#include "Resource/ConstantBufferResource/ConstantBufferResource.hpp"

namespace Detail
{
	/// @brief レンダーリソースストア
	class ResourceStore
	{
    public:

        /// @brief 任意のリソースを生成・登録する
        /// @tparam ResType 
        /// @tparam ...Args 
        /// @param ...args 
        /// @return 
        template<class ResType, class... Args>
        int32_t Create(Args&&... args)
        {
            auto typeIdx = std::type_index(typeid(ResType));
            auto& freeQueue = freeHandles_[typeIdx];

            // 空きハンドルがあれば再利用（オブジェクト自体は再生成せず中身を使い回す想定）
            if (!freeQueue.empty())
            {
                int32_t reusedHandle = freeQueue.front();
                freeQueue.pop();
                return reusedHandle;
            }

            // 新規作成
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
            if (handle >= 0 && handle < resources_.size() && resources_[handle].resource)
            {
                // IResourceから要求された型へダウンキャスト
                return static_cast<ResType*>(resources_[handle].resource.get());
            }

            return nullptr;
        }

    private:

        // リソースエントリ構造体
        struct ResourceEntry
        {
            std::unique_ptr<IResource> resource;
            std::type_index type;
        };

        // リソースを一元管理
        std::vector<ResourceEntry> resources_;

        // 型情報に基づいて空きハンドルのキューを管理
        std::unordered_map<std::type_index, std::queue<int32_t>> freeHandles_;
	};
}
