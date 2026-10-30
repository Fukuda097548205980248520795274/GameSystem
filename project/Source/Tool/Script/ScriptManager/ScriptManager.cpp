#include "ScriptManager.hpp"
#include "Engine.hpp"
#include <cassert>
#include <iostream>

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
	auto registry = Engine::GetInstance()->GetRegistryECS();
    auto activeEntities = registry->GetActiveEntities();

	// スクリプトオブジェクトの参照カウントを解放
    for (Entity entity : activeEntities)
    {
        if (registry->HasComponent<ScriptComponent>(entity))
        {
			// スクリプトコンポーネントを取得
            auto scriptComp = registry->GetComponent<ScriptComponent>(entity);
            if (scriptComp->scriptObject)
            {
				// C++ 側で保持している参照カウントを解放
                scriptComp->scriptObject->Release();
                scriptComp->scriptObject = nullptr;

				// メソッドの参照のnullptr化
				scriptComp->initFunc = nullptr;
				scriptComp->updateFunc = nullptr;
            }
        }
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
    // スクリプトファイルを開く
    std::ifstream file(scriptPath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "[Error] ファイルが開けません。パスを確認してください: " << scriptPath << std::endl;
        return false;
    }

    // ファイルの中身を文字列として読み込む
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string scriptCode = buffer.str();
    file.close();

    CScriptBuilder builder;
    int r = builder.StartNewModule(scriptEngine_, moduleName.c_str());
    if (r < 0) return false;

    // ファイルパスの代わりに、メモリ上の文字列としてスクリプトを渡す
    r = builder.AddSectionFromMemory(scriptPath.c_str(), scriptCode.c_str(), static_cast<uint32_t>(scriptCode.length()));
    if (r < 0)
    {
        std::cerr << "[Error] AddSectionFromMemory 失敗。エラーコード: " << r << std::endl;
        return false;
    }

    r = builder.BuildModule();
    return r >= 0;
}

/// @brief C++ エンジンAPIのバインド
void Detail::ScriptManager::RegisterEngineAPI()
{
	// Vector3 の登録
	if (scriptEngine_->RegisterObjectType("Vector3", sizeof(Vector3), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vector3>()) < 0)
	{
		std::cerr << "[Error] Vector3 の登録に失敗しました。" << std::endl;
	}
    if(scriptEngine_->RegisterObjectProperty("Vector3", "float x", asOFFSET(Vector3, x)) < 0)
    {
        std::cerr << "[Error] Vector3 のプロパティ x の登録に失敗しました。" << std::endl;
    }
    if(scriptEngine_->RegisterObjectProperty("Vector3", "float y", asOFFSET(Vector3, y)) < 0)
    {
        std::cerr << "[Error] Vector3 のプロパティ y の登録に失敗しました。" << std::endl;
    }
    if(scriptEngine_->RegisterObjectProperty("Vector3", "float z", asOFFSET(Vector3, z)) < 0)
    {
        std::cerr << "[Error] Vector3 のプロパティ z の登録に失敗しました。" << std::endl;
    }

	// Vector2 の登録
	if (scriptEngine_->RegisterObjectType("Vector2", sizeof(Vector2), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vector2>()) < 0)
	{
		std::cerr << "[Error] Vector2 の登録に失敗しました。" << std::endl;
	}
    if(scriptEngine_->RegisterObjectProperty("Vector2", "float x", asOFFSET(Vector2, x)) < 0)
    {
        std::cerr << "[Error] Vector2 のプロパティ x の登録に失敗しました。" << std::endl;
    }
    if(scriptEngine_->RegisterObjectProperty("Vector2", "float y", asOFFSET(Vector2, y)) < 0)
    {
        std::cerr << "[Error] Vector2 のプロパティ y の登録に失敗しました。" << std::endl;
    }

	// TransformComponent の登録
    if(scriptEngine_->RegisterObjectType("TransformComponent", sizeof(TransformComponent), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<TransformComponent>()) < 0)
    {
        std::cerr << "[Error] TransformComponent の登録に失敗しました。" << std::endl;
    }
    if(scriptEngine_->RegisterObjectProperty("TransformComponent", "Vector3 position", asOFFSET(TransformComponent, position)) < 0)
    {
        std::cerr << "[Error] TransformComponent のプロパティ position の登録に失敗しました。" << std::endl;
    }
    if(scriptEngine_->RegisterObjectProperty("TransformComponent", "Vector3 rotation", asOFFSET(TransformComponent, rotation)) < 0)
    {
        std::cerr << "[Error] TransformComponent のプロパティ rotation の登録に失敗しました。" << std::endl;
    }
    if(scriptEngine_->RegisterObjectProperty("TransformComponent", "Vector3 scale", asOFFSET(TransformComponent, scale)) < 0)
    {
        std::cerr << "[Error] TransformComponent のプロパティ scale の登録に失敗しました。" << std::endl;
    }

	// Log 関数の登録
    if(scriptEngine_->RegisterGlobalFunction("void Log(const string &in)",
        asFUNCTIONPR([](const std::string& str) 
            {
                Engine::GetInstance()->Log(LogLevel::Info, str);
            }, 
            (const std::string&), void), asCALL_CDECL) < 0)
    {
        std::cerr << "[Error] Log 関数の登録に失敗しました。" << std::endl;
    }
}