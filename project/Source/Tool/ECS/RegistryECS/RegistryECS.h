#pragma once
#include <unordered_map>
#include <memory>
#include <typeindex>

#include "ECS/EntityECS/EntityECS.h"

namespace Detail
{
	class RegistryECS
	{
	public:

		/// @brief エンティティを作成する
		/// @return 
		Entity CreateEntity();

		/// @brief エンティティを破棄する
		/// @param entity 
		void DestroyEntity(Entity entity);

		/// @brief コンポーネントを追加する
		/// @tparam T 
		/// @param entity 
		/// @param component 
		template<typename T>
		void AddComponent(Entity entity, T component)
		{
			GetComponentArray<T>()->Insert(entity, component);
		}

		/// @brief コンポーネントを取得する
		/// @tparam T 
		/// @param entity 
		/// @return 
		template<typename T>
		T& GetComponent(Entity entity)
		{
			return GetComponentArray<T>()->Get(entity);
		}

		/// @brief コンポーネント配列を取得する
		/// @tparam T 
		/// @return 
		template<typename T>
		ComponentArray<T>* GetComponentArray()
		{
			std::type_index typeIndex(typeid(T));
			if (componentArrays_.find(typeIndex) == componentArrays_.end())
			{
				componentArrays_[typeIndex] = std::make_unique<ComponentArray<T>>();
			}
			return static_cast<ComponentArray<T>*>(componentArrays_[typeIndex].get());
		}

		/// @brief エンティティがコンポーネントを持っているか確認する
		/// @tparam T 
		/// @param entity 
		/// @return 
		template<typename T>
		bool HasComponent(Entity entity)
		{
			auto componentArray = GetComponentArray<T>();
			return componentArray->Has(entity);
		}

		/// @brief 有効なエンティティの一覧を取得する
		/// @return 
		const std::vector<Entity>& GetActiveEntities() const { return activeEntities_; }


	private:

		/// @brief 次のエンティティID
		Entity nextEntityId_ = 0;

		/// @brief コンポーネント配列のマップ
		std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> componentArrays_;

		/// @brief 有効なエンティティの一覧
		std::vector<Entity> activeEntities_;
	};
}