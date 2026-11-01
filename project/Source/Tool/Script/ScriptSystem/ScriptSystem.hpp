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

		/// @brief スクリプトエンジンを登録する
		/// @param moduleName 
		/// @param scriptClass 
		/// @param scriptPath 
		/// @return 
		ScriptHandle BuildScriptModule(const std::string& moduleName, const std::string& scriptClass, const std::string& scriptPath) { return scriptManager_->BuildScriptModule(moduleName, scriptClass, scriptPath); }

		/// @brief スクリプトマネージャを取得する
		/// @return 
		ScriptManager* GetScriptManager() const { return scriptManager_.get(); }


	private:

		/// @brief 初期化
		void Initialize();

		// @brief スクリプトマネージャ
		std::unique_ptr<ScriptManager> scriptManager_;

	};
}