#include "RegistryECS.h"

/// @brief エンティティを作成する
/// @return 
Entity Detail::RegistryECS::CreateEntity()
{
	Entity id = nextEntityId_++;
	activeEntities_.push_back(id);
	return id;
}

/// @brief エンティティを破棄する
/// @param entity 
void Detail::RegistryECS::DestroyEntity(Entity entity)
{
	for (auto& [type, array] : componentArrays_)
	{
		array->RemoveEntity(entity);
	}
	
	auto it = std::find(activeEntities_.begin(), activeEntities_.end(), entity);
	if (it != activeEntities_.end())
	{
		activeEntities_.erase(it);
	}
}