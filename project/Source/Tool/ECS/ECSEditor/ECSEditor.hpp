#pragma once
#include "ECS/RegistryECS/RegistryECS.hpp"
#include "ECS/ComponentECS/ComponentECS.hpp"

namespace Detail
{
	/// @brief ECSエディタ
	class ECSEditor
	{
	public:

		/// @brief コンストラクタ
		ECSEditor() { Initialize(); }

		/// @brief デストラクタ
		~ECSEditor() = default;

		/// @brief 更新
		/// @param registry 
		void Update();


	private:

		/// @brief 初期化
		void Initialize();

		/// @brief ヒエラルキーの描画 
		/// @param registry 
		void DrawHierarchy();

		/// @brief インスペクターの描画
		/// @param registry 
		void DrawInspector();

		/// @brief エンティティコンポーネントUIの描画
		/// @param entity
		void DrawEntityComponents(Entity entity);

		/// @brief プレイコントロールの描画
		void DrawPlayControls();


	private:

		/// @brief 選択中のエンティティ
		Entity selectedEntity_ = kNullEntity;

		/// @brief レジストリ
		RegistryECS* registry_ = nullptr;

		// 状態保存用のレジストリ
		std::unique_ptr<RegistryECS> backupRegistry_ = nullptr;
	};
}