#pragma once
#include <angelscript.h>
#include <string>

namespace Detail
{
    namespace ScriptRegisterAPI
    {
        /// @brief 数学APIをスクリプトエンジンに登録する
        /// @param scriptEngine 
        void RegisterMathAPI(asIScriptEngine* scriptEngine);

        /// @brief コンポーネントAPIをスクリプトエンジンに登録する
        /// @param scriptEngine 
        void RegisterComponentAPI(asIScriptEngine* scriptEngine);

        /// @brief エンジンAPIをスクリプトエンジンに登録する
        /// @param scriptEngine 
        void RegisterEngineAPI(asIScriptEngine* scriptEngine);

        /// @brief 入力ボタンAPIをスクリプトエンジンに登録する
        /// @param scriptEngine 
        void RegisterInputButtonAPI(asIScriptEngine* scriptEngine);

        /// @brief 入力処理APIをスクリプトエンジンに登録する
        /// @param scriptEngine 
        void RegisterInputAPI(asIScriptEngine* scriptEngine);
    }
}
