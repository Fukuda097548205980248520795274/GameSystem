#pragma once
#include "ECS/RegistryECS/RegistryECS.h"
#include "ECS/ComponentECS/ComponentECS.h"

namespace Detail
{
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


	private:

		/// @brief 選択中のエンティティ
		Entity selectedEntity_ = kNullEntity;

		/// @brief レジストリ
		RegistryECS* registry_ = nullptr;
	};
}