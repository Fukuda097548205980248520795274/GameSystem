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

			// 例外コールバック
			ctx->SetExceptionCallback(asFUNCTION(Detail::ScriptSystem::ExceptionCallback), nullptr, asCALL_CDECL);

#ifdef DEVELOPMENT
			// デバッグ用のラインコールバック
			auto startTime = std::chrono::steady_clock::now();
			ctx->SetLineCallback(asFUNCTION(Detail::ScriptSystem::LineCallback), &startTime, asCALL_CDECL);
#endif

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

				// 例外コールバックを設定
				initCtx->SetExceptionCallback(asFUNCTION(Detail::ScriptSystem::ExceptionCallback), nullptr, asCALL_CDECL);

#ifdef DEVELOPMENT
				// デバッグ用のラインコールバック
				auto initStartTime = std::chrono::steady_clock::now();
				initCtx->SetLineCallback(asFUNCTION(Detail::ScriptSystem::LineCallback), &initStartTime, asCALL_CDECL);
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

			// 例外コールバックを設定
			ctx->SetExceptionCallback(asFUNCTION(Detail::ScriptSystem::ExceptionCallback), nullptr, asCALL_CDECL);

#ifdef DEVELOPMENT
			// デバッグ用のラインコールバック
			auto startTime = std::chrono::steady_clock::now();
			ctx->SetLineCallback(asFUNCTION(Detail::ScriptSystem::LineCallback), &startTime, asCALL_CDECL);
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
				const char* scriptSection;
				int line = ctx->GetLineNumber(0, nullptr, &scriptSection);
				engine->Log(LogLevel::Error, "スクリプトの実行がタイムアウトしました (無限ループの可能性)。ファイル : "
					+ std::string(scriptSection) + ", 行番号: " + std::to_string(line));
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