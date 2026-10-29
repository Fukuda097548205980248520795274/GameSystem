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

            scriptComp->scriptObject = *static_cast<asIScriptObject**>(ctx->GetAddressOfReturnValue());
            scriptComp->scriptObject->AddRef(); // 参照カウント増加
            ctx->Release();

            // メソッドの取得
            scriptComp->initFunc = type->GetMethodByDecl("void Initialize()");
            scriptComp->updateFunc = type->GetMethodByDecl("void Update(float)");

            // Initialize の実行
            if (scriptComp->initFunc)
            {
                asIScriptContext* initCtx = scriptEngine->CreateContext();
                initCtx->Prepare(scriptComp->initFunc);
                initCtx->SetObject(scriptComp->scriptObject);
                initCtx->Execute();
                initCtx->Release();
            }
        }

        // Update メソッドの実行
        if (scriptComp->scriptObject && scriptComp->updateFunc)
        {
            asIScriptContext* ctx = scriptEngine->CreateContext();
            ctx->Prepare(scriptComp->updateFunc);
            ctx->SetObject(scriptComp->scriptObject);
            ctx->SetArgFloat(0, 0.016f); // デルタタイム等を渡す
            ctx->Execute();
            ctx->Release();
        }
    }
}