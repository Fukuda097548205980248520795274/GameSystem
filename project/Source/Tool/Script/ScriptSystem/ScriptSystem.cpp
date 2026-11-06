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
        if (!registry->HasComponent<ScriptComponent>(entity)) continue;

		// スクリプトコンポーネントを取得
        auto scriptComp = registry->GetComponent<ScriptComponent>(entity);

		// スクリプトハンドルが無効な場合はスキップ
		if (scriptComp->handle == Detail::ScriptHandle::Invalid()) continue;

        // スクリプトインスタンスの生成（初回のみ）
        if (!scriptComp->scriptObject)
        {
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
            asIScriptContext* ctx = scriptEngine->CreateContext();
            ctx->Prepare(factory);
            ctx->Execute();

			// スクリプトオブジェクトの取得と参照カウントの増加
            scriptComp->scriptObject = static_cast<asIScriptObject*>(ctx->GetReturnObject());
            if (scriptComp->scriptObject)scriptComp->scriptObject->AddRef(); // C++側で保持するために参照カウントを増やす
            ctx->Release();

            // メソッドの取得
            scriptComp->initFunc = type->GetMethodByDecl("void Initialize()");
            scriptComp->updateFunc = type->GetMethodByDecl("void Update()");

            // Initialize の実行
            if (scriptComp->initFunc)
            {
                asIScriptContext* initCtx = scriptEngine->CreateContext();
                initCtx->Prepare(scriptComp->initFunc);
                initCtx->SetObject(scriptComp->scriptObject);

#ifdef DEVELOPMENT
				// デバッグ用のラインコールバックを設定
				initCtx->SetLineCallback(asFUNCTION(Detail::ScriptSystem::LineCallback), nullptr, asCALL_CDECL);
				initCtx->SetExceptionCallback(asFUNCTION(Detail::ScriptSystem::ExceptionCallback), nullptr, asCALL_CDECL);
#endif
                
                int r = initCtx->Execute();
                if (r != asEXECUTION_FINISHED)
                {
                    Engine::GetInstance()->Log(LogLevel::Error, "スクリプトの Initialize 実行時にエラーが発生しました");
                    Engine::GetInstance()->StopExecution();
                }

                initCtx->Release();
            }
        }

        // Update メソッドの実行
        if (scriptComp->scriptObject && scriptComp->updateFunc)
        {
            asIScriptContext* ctx = scriptEngine->CreateContext();
            ctx->Prepare(scriptComp->updateFunc);
            ctx->SetObject(scriptComp->scriptObject);

#ifdef DEVELOPMENT
			// デバッグ用のラインコールバックを設定
            ctx->SetLineCallback(asFUNCTION(Detail::ScriptSystem::LineCallback), nullptr, asCALL_CDECL);
            ctx->SetExceptionCallback(asFUNCTION(Detail::ScriptSystem::ExceptionCallback), nullptr, asCALL_CDECL);
#endif
            
            int r = ctx->Execute();
            if (r != asEXECUTION_FINISHED)
            {
                Engine::GetInstance()->Log(LogLevel::Error, "スクリプトの Update 実行時にエラーが発生しました");
                Engine::GetInstance()->StopExecution();
            }

            ctx->Release();
        }
    }
}

/// @brief デバッグ用コールバック
/// @param ctx 
/// @param timeOut 
void Detail::ScriptSystem::LineCallback(asIScriptContext* ctx, void* timeOut)
{
    auto engine = Engine::GetInstance();
    if (engine)
    {
        // 現在実行中のファイル名と行番号を取得
        const char* scriptSection;
        int line = ctx->GetLineNumber(0, nullptr, &scriptSection);

        engine->Log(LogLevel::Error, "スクリプト実行中, ファイル : " + std::string(scriptSection) + ", 行番号: " + std::to_string(line));
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
        const char* section;

        // 例外が発生した行番号とファイル名を取得
        int line = ctx->GetExceptionLineNumber(nullptr, &section);

        // 例外の理由（エラーメッセージ）を取得
        std::string err = ctx->GetExceptionString();

        engine->Log(LogLevel::Error, "スクリプト例外発生: " + err + " (ファイル: " + std::string(section) + ", 行: " + std::to_string(line) + ")");
    }
}