#pragma once
#include <vector>
#include <cstdint>
#include <cassert>

// @brief エンティティの型
using Entity = uint32_t;

// @brief 無効なエンティティの定数
constexpr Entity kNullEntity = UINT32_MAX;

/// @brief コンポーネント配列のインターフェース
class IComponentArray
{
public:

	/// @brief 仮想デストラクタ
	~IComponentArray() = default;

	/// @brief エンティティを削除する
	/// @param entity 
	virtual void RemoveEntity(Entity entity) = 0;
};

/// @brief コンポーネント配列
/// @tparam T 
template<typename T>
class ComponentArray : public IComponentArray
{
public:

	/// @brief コンポーネントを追加する
	/// @param entity 
	/// @param component 
	void Insert(Entity entity, T component)
	{
		// エンティティがすでに存在する場合は、コンポーネントを更新する
		if (entity >= sparse_.size())
			sparse_.resize(entity + 1, kNullEntity);

		// インデックスを取得し、スパース配列にエンティティを登録する
		size_t index = dense_.size();
		sparse_[entity] = static_cast<uint32_t>(index);
		denseToEntity_.push_back(entity);
		dense_.push_back(component);
	}

	/// @brief エンティティを削除する
	/// @param entity 
	void RemoveEntity(Entity entity) override
	{
		// エンティティが存在しない場合は何もしない
		if (!Has(entity)) return;

		uint32_t indexOfRemoved = sparse_[entity];
		uint32_t indexOfLast = static_cast<uint32_t>(dense_.size() - 1);

		// 末尾の要素を削除位置に移動
		dense_[indexOfRemoved] = std::move(dense_[indexOfLast]);
		Entity entityOfLast = denseToEntity_[indexOfLast];
		denseToEntity_[indexOfRemoved] = entityOfLast;
		sparse_[entityOfLast] = indexOfRemoved;

		// 末尾の要素を削除
		dense_.pop_back();
		denseToEntity_.pop_back();
		sparse_[entity] = kNullEntity;
	}

	/// @brief コンポーネントを取得する
	/// @param entity 
	/// @return 
	T& Get(Entity entity)
	{
		assert(Has(entity));
		return dense_[sparse_[entity]];
	}

	/// @brief エンティティが存在するか確認する
	/// @param entity 
	/// @return 
	bool Has(Entity entity) const
	{
		return entity < sparse_.size() && sparse_[entity] != kNullEntity;
	}

	/// @brief データを取得する
	/// @return 
	std::vector<T>& GetData() { return dense_; }

	/// @brief エンティティを取得する
	/// @return 
	const std::vector<Entity>& GetEntities() const { return denseToEntity_; }


private:

	// エンティティに対応するコンポーネントを保持する配列
	std::vector<T> dense_;
	
	// エンティティのインデックスを保持する配列
	std::vector<Entity> denseToEntity_;

	// スパース配列
	std::vector<uint32_t> sparse_;
};