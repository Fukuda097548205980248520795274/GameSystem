#pragma once
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <queue>

#include "ECS/EntityECS/EntityECS.h"

namespace Detail
{
	/// @brief ECSレジストリ
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
		T* GetComponent(Entity entity)
		{
			if (HasComponent<T>(entity))
				return &GetComponentArray<T>()->Get(entity);

			// コンポーネントが存在しない場合は nullptr を返す
			return nullptr;
		}

		/// @brief　コンポーネントを取得する（const版）
		/// @tparam T 
		/// @param entity 
		/// @return 
		template<typename T>
		const T* GetComponent(Entity entity) const
		{
			if (HasComponent<T>(entity))
				return &GetComponentArray<T>()->Get(entity);

			// コンポーネントが存在しない場合は nullptr を返す
			return nullptr;
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

		/// @brief エンティティが削除されたときに呼ばれるコールバックを設定する
		/// @tparam T 
		/// @param callback 
		template<typename T>
		void SetOnRemoveComponentCallback(typename ComponentArray<T>::OnRemoveCallback callback)
		{
			GetComponentArray<T>()->SetOnRemoveCallback(callback);
		}

		/// @brief 指定されたコンポーネントを1つ以上持つエンティティに対して処理を行う
		/// @tparam ...Components 
		/// @tparam Func 
		/// @param func 
		template<typename... Components, typename Func>
		void ForEachEntityWithAny(Func&& func)
		{
			// 各コンポーネントの配列を取得する
			auto arrays = std::tuple{ GetComponentArray<Components>()... };

			// 有効なエンティティの一覧をループする
			for (Entity entity : activeEntities_)
			{
				// 各コンポーネントの配列から、エンティティが持つコンポーネントへのポインタを取得する
				auto components = std::apply(
					[&](auto*... array) {
						return std::tuple<Components*...>{(array->Has(entity) ? &array->Get(entity) : nullptr)...};
					}, arrays);

				// 1つでもコンポーネントを持っているか確認する
				const bool hasAny = std::apply(
					[](auto*... component) {return ((component != nullptr) || ...);}, components);

				// 指定されたコンポーネントを1つも持っていない場合はスキップする
				if (!hasAny)continue;

				// Entity と各コンポーネントへのポインタを渡す
				std::apply([&](auto*... component) {func(entity, component...);}, components);
			}
		}

		/// @brief コンポーネントを削除する
		/// @tparam T 
		/// @param entity 
		template<typename T>
		void RemoveComponent(Entity entity)
		{
			// ComponentArray に実装されているであろう RemoveEntity を呼び出す
			GetComponentArray<T>()->RemoveEntity(entity);
		}


	private:

		/// @brief 次のエンティティID
		Entity nextEntityId_ = 0;

		/// @brief コンポーネント配列のマップ
		std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> componentArrays_;

		/// @brief 有効なエンティティの一覧
		std::vector<Entity> activeEntities_;

		/// @brief 利用可能なエンティティの一覧
		std::queue<Entity> availableEntities_;
	};
}