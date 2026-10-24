#include "RegistryECS.h"
#include "../ComponentECS/ComponentECS.h"

/// @brief エンティティを作成する
/// @return 
Entity Detail::RegistryECS::CreateEntity()
{
	Entity id;

	// 削除されて再利用可能なIDがあればそれを使用する
	if (!availableEntities_.empty())
	{
		id = availableEntities_.front();
		availableEntities_.pop();
	}
	else
	{
		// 再利用可能なIDがなければ新しいIDを割り当てる
		id = nextEntityId_++;
	}

	activeEntities_.push_back(id);
	return id;
}

/// @brief エンティティを破棄する
/// @param entity 
void Detail::RegistryECS::DestroyEntity(Entity entity)
{
	// 階層構造を持つエンティティの場合、子エンティティも再帰的に削除する
	if (HasComponent<HierarchyComponent>(entity))
	{
		auto hierarchy = GetComponent<HierarchyComponent>(entity);
		if (hierarchy)
		{
			// 子エンティティも再帰的に削除
			for (Entity child : hierarchy->children)
			{
				DestroyEntity(child);
			}

			// 親エンティティの子リストから削除する
			if (hierarchy->parent != kNullEntity && HasComponent<HierarchyComponent>(hierarchy->parent))
			{
				auto parentHierarchy = GetComponent<HierarchyComponent>(hierarchy->parent);
				if (parentHierarchy)
				{
					auto it = std::find(parentHierarchy->children.begin(), parentHierarchy->children.end(), entity);
					if (it != parentHierarchy->children.end())
					{
						parentHierarchy->children.erase(it);
					}
				}
			}
		}
	}

	// すべてのコンポーネント配列からエンティティを削除する
	for (auto& [type, array] : componentArrays_)
	{
		array->RemoveEntity(entity);
	}

	// 有効なエンティティの一覧からエンティティを削除する
	auto it = std::find(activeEntities_.begin(), activeEntities_.end(), entity);
	if (it != activeEntities_.end())
	{
		activeEntities_.erase(it);

		// 削除したエンティティのIDを再利用キューに追加する
		availableEntities_.push(entity);
	}
}