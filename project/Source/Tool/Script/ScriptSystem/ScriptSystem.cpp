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

        auto scriptComp = registry->GetComponent<ScriptComponent>(entity);

        // スクリプトインスタンスの生成（初回のみ）
        if (!scriptComp->scriptObject)
        {
			// スクリプトモジュールを取得
            asIScriptModule* mod = scriptEngine->GetModule(scriptComp->moduleName.c_str());
            if (!mod) continue;

			// スクリプトクラスの型情報を取得
            asITypeInfo* type = mod->GetTypeInfoByName(scriptComp->scriptClass.c_str());
            if (!type) continue;

            // コンストラクタ呼び出し
            asIScriptFunction* factory = type->GetFactoryByDecl((scriptComp->scriptClass + "@ " + scriptComp->scriptClass + "()").c_str());
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