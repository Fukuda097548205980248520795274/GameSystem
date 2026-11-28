#include "ScriptSystem.hpp"
#include "Engine.hpp"

/// @brief 初期化
void Detail::ScriptSystem::Initialize()
{
	// スクリプトマネージャを作成
	scriptManager_ = std::make_unique<ScriptManager>();
}

/// @brief 更新処理
/// @param registry 
void Detail::ScriptSystem::Update(RegistryECS* registry)
{
    // 有効なエンティティを取得
    auto activeEntities = registry->GetActiveEntities();

    // スクリプトエンジンを取得
    auto scriptEngine = scriptManager_->GetScriptEngine();

    for (Entity entity : activeEntities)
    {
        // プレハブ（雛形）自体はスクリプトを実行しない
        if (registry->HasComponent<PrefabComponent>(entity)) continue;

        // スクリプトコンポーネントを持っていない場合はスキップ
        if (!registry->HasComponent<ScriptComponent>(entity)) continue;

        // スクリプトコンポーネントを取得
        auto scriptComp = registry->GetComponent<ScriptComponent>(entity);

        // スクリプトハンドルが無効な場合はスキップ
        if (!scriptComp->handle.IsValid()) continue;

        // スクリプトインスタンスの生成（初回のみ）
        if (!scriptComp->scriptObject)
        {
            // スクリプトがロードされていない場合はスキップ
            if (!scriptManager_->EnsureLoaded(scriptComp->handle))continue;

            // スクリプトモジュールを取得
            std::string moduleName = scriptManager_->GetModuleName(scriptComp->handle);
            asIScriptModule* mod = scriptEngine->GetModule(moduleName.c_str());
            if (!mod) continue;

            // スクリプトクラスの型情報を取得
            std::string className = scriptManager_->GetScriptClass(scriptComp->handle);
            asITypeInfo* type = mod->GetTypeInfoByName(className.c_str());
            if (!type) continue;

            // コンストラクタ呼び出し
            asIScriptFunction* factory = type->GetFactoryByDecl((className + "@ " + className + "()").c_str());

            // コンストラクタが見つからない場合はエラーログを出力してスキップ
            if (!factory)
            {
                Engine::GetInstance()->Log(LogLevel::Error, "スクリプトのコンストラクタが見つかりません: " + className);
                continue;
            }

            asIScriptContext* ctx = scriptEngine->RequestContext();
            ctx->Prepare(factory);

            // エンティティIDをスクリプト側に渡すためにユーザーデータとして設定
            ctx->SetUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)));

            // 例外コールバック
            ctx->SetExceptionCallback(asFUNCTION(Detail::ScriptSystem::ExceptionCallback), nullptr, asCALL_CDECL);

#ifdef DEVELOPMENT
            // デバッグ用のラインコールバック
            auto startTime = std::chrono::steady_clock::now();
            ctx->SetLineCallback(asFUNCTION(Detail::ScriptSystem::LineCallback), &startTime, asCALL_CDECL);
#endif

            // コンストラクタ
            int r = ctx->Execute();
            bool hasError = (r != asEXECUTION_FINISHED);
            if (hasError)
            {
                Engine::GetInstance()->Log(LogLevel::Error, "スクリプトのコンストラクタ実行時にエラーが発生しました");
                Engine::GetInstance()->StopExecution();
            }
            else
            {
                // 正常終了時のみ取得
                scriptComp->scriptObject = static_cast<asIScriptObject*>(ctx->GetReturnObject());
                if (scriptComp->scriptObject) scriptComp->scriptObject->AddRef();
            }

#ifdef DEVELOPMENT

            // デバッグ用のラインコールバックをクリア
            ctx->ClearLineCallback();
#endif

            // コンテキストを返却
            scriptEngine->ReturnContext(ctx);

            // エラー発生時は Initialize や他エンティティの Update に進まず即座に終了
            if (hasError)
            {
                return;
            }


            // スクリプトのプロパティを保存している場合は、スクリプトオブジェクトに反映する
            if (scriptComp->scriptObject)
            {
                asITypeInfo* objType = scriptComp->scriptObject->GetObjectType();
                for (asUINT i = 0; i < objType->GetPropertyCount(); i++)
                {
                    const char* propName;
                    int typeId;
                    objType->GetProperty(i, &propName, &typeId, nullptr, nullptr, nullptr, nullptr, nullptr);
                    void* propAddr = scriptComp->scriptObject->GetAddressOfProperty(i);

                    if (!propAddr) continue;

                    if (typeId == asTYPEID_INT32 && scriptComp->savedIntProperties.count(propName))
                    {
                        *(int*)propAddr = scriptComp->savedIntProperties[propName];
                    }
                    else if (typeId == asTYPEID_FLOAT && scriptComp->savedFloatProperties.count(propName))
                    {
                        *(float*)propAddr = scriptComp->savedFloatProperties[propName];
                    }
                    else if (typeId == asTYPEID_BOOL && scriptComp->savedBoolProperties.count(propName))
                    {
                        *(bool*)propAddr = scriptComp->savedBoolProperties[propName];
                    }
                    else if (typeId == scriptEngine->GetTypeIdByDecl("Vector2") && scriptComp->savedVector2Properties.count(propName))
                    {
                        *(Vector2*)propAddr = scriptComp->savedVector2Properties[propName];
                    }
                    else if (typeId == scriptEngine->GetTypeIdByDecl("Vector3") && scriptComp->savedVector3Properties.count(propName))
                    {
                        *(Vector3*)propAddr = scriptComp->savedVector3Properties[propName];
                    }
                    else if (typeId == scriptEngine->GetTypeIdByDecl("string") && scriptComp->savedStringProperties.count(propName))
                    {
                        *(std::string*)propAddr = scriptComp->savedStringProperties[propName];
                    }
                    else if (typeId == scriptEngine->GetTypeIdByDecl("Entity") && scriptComp->savedEntityProperties.count(propName))
                    {
                        *(Entity*)propAddr = scriptComp->savedEntityProperties[propName];
                    }
                }
            }

            // メソッドの取得
            scriptComp->initFunc = type->GetMethodByDecl("void Initialize()");
            scriptComp->updateFunc = type->GetMethodByDecl("void Update()");

            // Initialize の実行
            if (scriptComp->initFunc)
            {
                asIScriptContext* initCtx = scriptEngine->RequestContext();
                initCtx->Prepare(scriptComp->initFunc);
                initCtx->SetObject(scriptComp->scriptObject);

                // エンティティIDをスクリプト側に渡すためにユーザーデータとして設定
                initCtx->SetUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)));

                // 例外コールバックを設定
                initCtx->SetExceptionCallback(asFUNCTION(Detail::ScriptSystem::ExceptionCallback), nullptr, asCALL_CDECL);

#ifdef DEVELOPMENT
                // デバッグ用のラインコールバック
                auto initStartTime = std::chrono::steady_clock::now();
                initCtx->SetLineCallback(asFUNCTION(Detail::ScriptSystem::LineCallback), &initStartTime, asCALL_CDECL);
#endif

                // Initialize メソッドの実行
                int r = initCtx->Execute();
                bool hasError = (r != asEXECUTION_FINISHED);
                if (hasError)
                {
                    Engine::GetInstance()->Log(LogLevel::Error, "スクリプトの初期化時にエラーが発生しました");
                    Engine::GetInstance()->StopExecution();
                }

#ifdef DEVELOPMENT
                // デバッグ用のラインコールバックをクリア
                initCtx->ClearLineCallback();
#endif

                // コンテキストを返却
                scriptEngine->ReturnContext(initCtx);

                // エラー発生時は Initialize や他エンティティの Update に進まず即座に終了
                if (hasError)
                {
                    return;
                }
            }
        }

        // Update メソッドの実行
        if (scriptComp->scriptObject && scriptComp->updateFunc)
        {
            asIScriptContext* ctx = scriptEngine->RequestContext();
            ctx->Prepare(scriptComp->updateFunc);
            ctx->SetObject(scriptComp->scriptObject);

            // エンティティIDをスクリプト側に渡すためにユーザーデータとして設定
            ctx->SetUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)));

            // 例外コールバックを設定
            ctx->SetExceptionCallback(asFUNCTION(Detail::ScriptSystem::ExceptionCallback), nullptr, asCALL_CDECL);

#ifdef DEVELOPMENT
            // デバッグ用のラインコールバック
            auto startTime = std::chrono::steady_clock::now();
            ctx->SetLineCallback(asFUNCTION(Detail::ScriptSystem::LineCallback), &startTime, asCALL_CDECL);
#endif

            // Update メソッドの実行
            int r = ctx->Execute();
            bool hasError = (r != asEXECUTION_FINISHED);
            if (hasError)
            {
                Engine::GetInstance()->Log(LogLevel::Error, "スクリプトの更新処理時にエラーが発生しました");
                Engine::GetInstance()->StopExecution();
            }

#ifdef DEVELOPMENT

            // デバッグ用のラインコールバックをクリア
            ctx->ClearLineCallback();
#endif

            // コンテキストを返却
            scriptEngine->ReturnContext(ctx);

            // エラー発生時は Initialize や他エンティティの Update に進まず即座に終了
            if (hasError)
            {
                return;
            }
        }
    }
}

/// @brief デバッグ用コールバック
/// @param ctx 
/// @param timeOut 
void Detail::ScriptSystem::LineCallback(asIScriptContext* ctx, void* timeOut)
{
    // timeOutポインタに開始時間を渡して、一定時間経過したら実行を中断する例
    if (timeOut)
    {
        auto startTime = *static_cast<std::chrono::steady_clock::time_point*>(timeOut);
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();


        // 3秒以上経過した場合にスクリプトの実行を中断
        if (elapsed > 3000)
        {
            ctx->Suspend(); // スクリプトの実行を中断させる

            auto engine = Engine::GetInstance();
            if (engine)
            {
                const char* scriptSection = nullptr;
                int line = ctx->GetLineNumber(0, nullptr, &scriptSection);
                std::string sectionName = scriptSection ? scriptSection : "Unknown";
                engine->Log(LogLevel::Error, "スクリプトの実行がタイムアウトしました (無限ループの可能性)。ファイル : "
                    + sectionName + ", 行番号: " + std::to_string(line));
            }
        }
    }
}

/// @brief 例外コールバック
/// @param ctx 
/// @param param 
void Detail::ScriptSystem::ExceptionCallback(asIScriptContext* ctx, void* param)
{
    auto engine = Engine::GetInstance();
    if (engine)
    {
        std::string err = ctx->GetExceptionString();
        engine->Log(LogLevel::Error, "スクリプト例外発生: " + err);

        // コールスタック（呼び出し履歴）を取得してログに出力
        engine->Log(LogLevel::Error, "--- コールスタック ---");
        asUINT stackSize = ctx->GetCallstackSize();
        for (asUINT i = 0; i < stackSize; i++)
        {
            asIScriptFunction* func = ctx->GetFunction(i);
            if (func)
            {
                const char* section;
                int line = ctx->GetLineNumber(i, nullptr, &section);

                std::string funcName = func->GetDeclaration();
                std::string sectionName = section ? section : "Unknown";

                engine->Log(LogLevel::Error, "  [" + std::to_string(i) + "] " + funcName
                    + " (ファイル: " + sectionName + ", 行: " + std::to_string(line) + ")");
            }
        }
        engine->Log(LogLevel::Error, "----------------------");
    }
}
