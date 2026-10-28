#include "ScriptManager.h"
#include "Engine.h"
#include <cassert>

/// @brief 初期化
void Detail::ScriptManager::Initialize()
{
    // エンジンの作成
    scriptEngine_ = asCreateScriptEngine();
    assert(scriptEngine_ != nullptr);

    // メッセージコールバックの設定（エラー・警告ログ）
    scriptEngine_->SetMessageCallback(asMETHOD(ScriptManager, MessageCallback), this, asCALL_THISCALL);

    // 標準アッドオンの登録 (std::string サポート)
    RegisterStdString(scriptEngine_);

    // C++ エンジンAPIのバインド
    RegisterEngineAPI();
}

/// @brief シャットダウン
void Detail::ScriptManager::Shutdown()
{
    if (scriptEngine_)
    {
        scriptEngine_->ShutDownAndRelease();
        scriptEngine_ = nullptr;
    }
}

/// @brief メッセージコールバック
/// @param msg 
void Detail::ScriptManager::MessageCallback(const asSMessageInfo* msg)
{
    LogLevel level = LogLevel::Info;
    if (msg->type == asMSGTYPE_ERROR) level = LogLevel::Error;
    else if (msg->type == asMSGTYPE_WARNING) level = LogLevel::Warning;

	// メッセージの整形
    std::string logMsg = std::string(msg->section) + " (" + std::to_string(msg->row) + ", " + std::to_string(msg->col) + ") : " + msg->message;
    Engine::GetInstance()->Log(level, logMsg);
}

/// @brief スクリプトエンジンを登録する
/// @param moduleName 
/// @param scriptPath 
/// @return 
bool Detail::ScriptManager::BuildScriptModule(const std::string& moduleName, const std::string& scriptPath)
{
    CScriptBuilder builder;
    int r = builder.StartNewModule(scriptEngine_, moduleName.c_str());
    if (r < 0) return false;

    r = builder.AddSectionFromFile(scriptPath.c_str());
    if (r < 0) return false;

    r = builder.BuildModule();
    return r >= 0;
}

/// @brief C++ エンジンAPIのバインド
void Detail::ScriptManager::RegisterEngineAPI()
{
	// Vector3 の登録
    scriptEngine_->RegisterObjectType("Vector3", sizeof(Vector3), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vector3>());
    scriptEngine_->RegisterObjectProperty("Vector3", "float x", asOFFSET(Vector3, x));
    scriptEngine_->RegisterObjectProperty("Vector3", "float y", asOFFSET(Vector3, y));
    scriptEngine_->RegisterObjectProperty("Vector3", "float z", asOFFSET(Vector3, z));

	// TransformComponent の登録
    scriptEngine_->RegisterObjectType("TransformComponent", sizeof(TransformComponent), asOBJ_VALUE | asGetTypeTraits<TransformComponent>());
    scriptEngine_->RegisterObjectProperty("TransformComponent", "Vector3 position", asOFFSET(TransformComponent, position));
    scriptEngine_->RegisterObjectProperty("TransformComponent", "Vector3 rotation", asOFFSET(TransformComponent, rotation));
    scriptEngine_->RegisterObjectProperty("TransformComponent", "Vector3 scale", asOFFSET(TransformComponent, scale));

	// Log 関数の登録
    scriptEngine_->RegisterGlobalFunction("void Log(const string &in)",
        asFUNCTIONPR([](const std::string& str) {Engine::GetInstance()->Log(LogLevel::Info, str);}, 
            (const std::string&), void), asCALL_CDECL);
}