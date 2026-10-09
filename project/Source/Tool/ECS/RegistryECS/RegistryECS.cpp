#include "RegistryECS.h"

/// @brief エンティティを作成する
/// @return 
Entity Detail::RegistryECS::CreateEntity()
{
	Entity id = nextEntityId_++;
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
}