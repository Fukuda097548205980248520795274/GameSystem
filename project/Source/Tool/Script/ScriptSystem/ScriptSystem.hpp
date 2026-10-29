#pragma once
#include <memory>
#include "../ScriptManager/ScriptManager.hpp"

namespace Detail
{
	class RegistryECS;

	class ScriptSystem
	{
	public:

		/// @brief コンストラクタ
		ScriptSystem() { Initialize(); }

		/// @brief デストラクタ
		~ScriptSystem() = default;

		/// @brief 更新処理
		/// @param registry 
		void Update(RegistryECS* registry);


	private:

		/// @brief 初期化
		void Initialize();

		// @brief スクリプトマネージャ
		std::unique_ptr<ScriptManager> scriptManager_;

	};
}