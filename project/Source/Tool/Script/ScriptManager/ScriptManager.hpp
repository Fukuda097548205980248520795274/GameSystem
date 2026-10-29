#pragma once
#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptstdstring/scriptstdstring.h>
#include <string>

namespace Detail
{
	/// @brief スクリプトマネージャ
	class ScriptManager
	{
    public:

        /// @brief コンストラクタ
		ScriptManager() {Initialize();}

        /// @brief デストラクタ
		~ScriptManager() { Shutdown(); }

		/// @brief スクリプトモジュールをビルドする
        /// @param moduleName 
        /// @param scriptPath 
        /// @return 
        bool BuildScriptModule(const std::string& moduleName, const std::string& scriptPath);

        /// @brief スクリプトエンジンを取得する
        /// @return 
        asIScriptEngine* GetScriptEngine() const { return scriptEngine_; }

    private:

        /// @brief 初期化
        void Initialize();

        /// @brief シャットダウン
        void Shutdown();

		/// @brief メッセージコールバック
        /// @param msg 
        void MessageCallback(const asSMessageInfo* msg);

		/// @brief スクリプトエンジンを登録する
        void RegisterEngineAPI();

    private:

        /// @brief スクリプトエンジン
        asIScriptEngine* scriptEngine_ = nullptr;
	};
}