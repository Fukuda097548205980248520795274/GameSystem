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
Detail::ScriptHandle Detail::ScriptManager::BuildScriptModule(const std::string& moduleName, const std::string& scriptPath)
{
    auto engine = Engine::GetInstance();

    std::filesystem::path asFilePath(scriptPath);
    if (!std::filesystem::exists(asFilePath))
    {
        if (engine) engine->Log(LogLevel::Error, "スクリプトファイルが存在しません : " + scriptPath);
        return ScriptHandle::Invalid();
    }

    // ファイルの更新日時を取得
    auto asTime = std::filesystem::last_write_time(asFilePath);

    // キャッシュチェック
    auto it = cache_.find(scriptPath);
    if (it != cache_.end())
    {
        // 既存のハンドルが有効か確認
        ScriptHandle cachedHandle = GetHandleByIndex(it->second.index);
        if (cachedHandle != ScriptHandle::Invalid())
        {
            // キャッシュされている更新日時と比較
            if (slots_[cachedHandle.index].lastWriteTime >= asTime)
            {
                // ファイルが更新されていなければキャッシュを返す
                return cachedHandle;
            }
            // 更新されている場合はそのまま再ビルドへ進む
        }
    }

    // スクリプトファイルのパスを解析して中間データ（bin）のパスを生成
    std::string binPath = kCookedDir + asFilePath.stem().string() + ".bin";

    // 中間データ用ディレクトリが存在しない場合は作成
    if (!std::filesystem::exists(kCookedDir))
        std::filesystem::create_directories(kCookedDir);

    // 中間データ（bin）を読み込むべきかのチェック
    bool loadFromBin = false;
    if (std::filesystem::exists(binPath))
    {
        auto binTime = std::filesystem::last_write_time(binPath);
        if (binTime >= asTime)
        {
            loadFromBin = true;
        }
    }

    bool isLoaded = false;

    if (loadFromBin)
    {
        // バイナリからロード
        asIScriptModule* mod = scriptEngine_->GetModule(moduleName.c_str(), asGM_ALWAYS_CREATE);
        if (mod)
        {
            CBytecodeStream stream(binPath, false);
            if (stream.IsOpen())
            {
                int r = mod->LoadByteCode(&stream);
                if (r >= 0)
                {
                    isLoaded = true;
                }
                else
                {
                    if (engine)
                    {
                        engine->Log(LogLevel::Error, "バイトコードの読み込みに失敗しました : " + binPath);
                        engine->StopExecution();
                    }
                }
            }
        }
    }

	
	if (engine)engine->Log(LogLevel::Info, "スクリプトモジュールのビルド : " + moduleName + (isLoaded ? " (バイトコードからロード)" : " (ソースからビルド)"));


    if (!isLoaded)
    {
        // スクリプトファイルを開く
        std::ifstream file(scriptPath, std::ios::binary);
        if (!file.is_open())
        {
            if (engine)
            {
                engine->Log(LogLevel::Error, "スクリプトファイルが開けません : " + scriptPath);
                engine->StopExecution();
            }
            return ScriptHandle::Invalid();
        }


        // ファイルの中身を文字列として読み込む
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string scriptCode = buffer.str();
        file.close();

        // スクリプトビルダーを使用してモジュールをビルド
        CScriptBuilder builder;
        int r = builder.StartNewModule(scriptEngine_, moduleName.c_str());
        if (r < 0)
        {
            if (engine) engine->StopExecution();
            return ScriptHandle::Invalid();
        }

        // スクリプトコードをメモリから追加
        r = builder.AddSectionFromMemory(scriptPath.c_str(), scriptCode.c_str(), static_cast<uint32_t>(scriptCode.length()));
        if (r < 0)
        {
            if (engine)
            {
                engine->Log(LogLevel::Error, "AddSectionFromMemory 失敗。エラーコード: " + std::to_string(r));
                engine->StopExecution();
            }
            return ScriptHandle::Invalid();
        }
        if (engine)engine->Log(LogLevel::Info, "スクリプトコードをメモリから追加 : " + scriptPath);

        // モジュールのビルド
        r = builder.BuildModule();
        if (r < 0)
        {
            if (engine)
            {
                engine->Log(LogLevel::Error, "BuildModule 失敗。エラーコード: " + std::to_string(r));
                engine->StopExecution();
            }
            return ScriptHandle::Invalid();
        }
		if (engine)engine->Log(LogLevel::Info, "スクリプトモジュールをビルド : " + moduleName);

        // ビルドに成功した場合、バイトコード（中間データ）として保存
        asIScriptModule* mod = builder.GetModule();
        if (mod)
        {
            CBytecodeStream stream(binPath, true);
            if (stream.IsOpen())
            {
                mod->SaveByteCode(&stream);
            }
        }
    }

    // スロットの割り当て
    uint32_t slotIndex = 0;
    if (it != cache_.end() && GetHandleByIndex(it->second.index) != ScriptHandle::Invalid())
    {
        // 既に有効なスロットがある場合は再利用する
        slotIndex = it->second.index;
    }
    else if (!freeSlots_.empty())
    {
        // 空いているスロットを再利用
        slotIndex = freeSlots_.front();
        freeSlots_.pop();
    }
    else
    {
        // 新規スロット作成
        slotIndex = static_cast<uint32_t>(slots_.size());
        slots_.emplace_back();
    }

    // スロットにデータを格納
    Slot& slot = slots_[slotIndex];
    slot.inUse = true;
    slot.moduleName = moduleName;
    slot.lastWriteTime = asTime; // ★更新日時を記録

    // キャッシュに登録
    ScriptHandle handle{ slotIndex, slot.generation };
    cache_[scriptPath] = handle;

    return handle;
}

/// @brief インデックスからハンドルを取得する
/// @param index 
/// @return 
Detail::ScriptHandle Detail::ScriptManager::GetHandleByIndex(uint32_t index) const
{
	// インデックスが範囲外、またはスロットが使用中でない場合は無効なハンドルを返す
    if (index >= slots_.size() || !slots_[index].inUse)
        return ScriptHandle::Invalid();

	return ScriptHandle{ index, slots_[index].generation };
}

/// @brief スクリプトを削除する
/// @param handle 
void Detail::ScriptManager::Remove(ScriptHandle handle)
{
    if (!handle.IsValid() || handle.index >= slots_.size()) return;

	Slot& slot = slots_[handle.index];

	// スロットが使用中で、世代番号が一致する場合のみ削除処理を行う
	if (slot.inUse && slot.generation == handle.generation)
	{
		// スクリプトモジュールを破棄
        if (scriptEngine_ && !slot.moduleName.empty())
        {
            scriptEngine_->DiscardModule(slot.moduleName.c_str());
        }

		slot.inUse = false;
		freeSlots_.push(handle.index);
	}

	// キャッシュから該当するスクリプトパスを削除
	for (auto it = cache_.begin(); it != cache_.end(); )
	{
		if (it->second.index == handle.index)
		{
			it = cache_.erase(it);
		}
		else
		{
			++it;
		}
	}
}

/// @brief スクリプトハンドルからスクリプトパスを取得する
/// @param handle 
/// @return 
std::string Detail::ScriptManager::GetFilePath(ScriptHandle handle) const
{
    if (!handle.IsValid()) return "";

	// キャッシュを検索して、ハンドルに対応するスクリプトパスを返す
    for (const auto& pair : cache_)
    {
        if (pair.second.index == handle.index && pair.second.generation == handle.generation)
        {
            return pair.first;
        }
    }

    return "";
}

/// @brief 変更されたスクリプトをリロードする
void Detail::ScriptManager::ReloadModifiedScripts()
{
    auto engine = Engine::GetInstance();
    for (auto& pair : cache_)
    {
        const std::string& scriptPath = pair.first;
        ScriptHandle handle = pair.second;

        if (!handle.IsValid() || handle.index >= slots_.size()) continue;

        Slot& slot = slots_[handle.index];
        if (!slot.inUse) continue;

        if (std::filesystem::exists(scriptPath))
        {
            auto currentWriteTime = std::filesystem::last_write_time(scriptPath);

            // キャッシュした時点の時刻より新しければ再ビルド
            if (currentWriteTime > slot.lastWriteTime)
            {
                if (engine) engine->Log(LogLevel::Info, "スクリプトの更新を検知しました。再ビルドします : " + scriptPath);

                // 再ビルド処理を呼び出す (内部でキャッシュスロットが上書き更新される)
                BuildScriptModule(slot.moduleName, scriptPath);
            }
        }
    }
}

/// @brief C++ エンジンAPIのバインド
void Detail::ScriptManager::RegisterEngineAPI()
{
	auto engine = Engine::GetInstance();

	// Vector3 の登録
	if (scriptEngine_->RegisterObjectType("Vector3", sizeof(Vector3), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vector3>()) < 0)
	{
        if(engine)engine->Log(LogLevel::Error, "Vector3 の登録に失敗しました");
	}
    if(scriptEngine_->RegisterObjectProperty("Vector3", "float x", asOFFSET(Vector3, x)) < 0)
    {
        if(engine)engine->Log(LogLevel::Error, "Vector3 のプロパティ x の登録に失敗しました");
    }
    if(scriptEngine_->RegisterObjectProperty("Vector3", "float y", asOFFSET(Vector3, y)) < 0)
    {
        if(engine)engine->Log(LogLevel::Error, "Vector3 のプロパティ y の登録に失敗しました");
    }
    if(scriptEngine_->RegisterObjectProperty("Vector3", "float z", asOFFSET(Vector3, z)) < 0)
    {
        if(engine)engine->Log(LogLevel::Error, "Vector3 のプロパティ z の登録に失敗しました");
    }

	// Vector2 の登録
	if (scriptEngine_->RegisterObjectType("Vector2", sizeof(Vector2), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vector2>()) < 0)
	{
		if (engine) engine->Log(LogLevel::Error, "Vector2 の登録に失敗しました");
	}
    if(scriptEngine_->RegisterObjectProperty("Vector2", "float x", asOFFSET(Vector2, x)) < 0)
    {
        if(engine)engine->Log(LogLevel::Error, "Vector2 のプロパティ x の登録に失敗しました");
    }
    if(scriptEngine_->RegisterObjectProperty("Vector2", "float y", asOFFSET(Vector2, y)) < 0)
    {
        if(engine)engine->Log(LogLevel::Error, "Vector2 のプロパティ y の登録に失敗しました");
    }

	// TransformComponent の登録
    if(scriptEngine_->RegisterObjectType("TransformComponent", sizeof(TransformComponent), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<TransformComponent>()) < 0)
    {
        if(engine)engine->Log(LogLevel::Error, "TransformComponent の登録に失敗しました");
    }
    if(scriptEngine_->RegisterObjectProperty("TransformComponent", "Vector3 position", asOFFSET(TransformComponent, position)) < 0)
    {
        if(engine)engine->Log(LogLevel::Error, "TransformComponent のプロパティ position の登録に失敗しました");
    }
    if(scriptEngine_->RegisterObjectProperty("TransformComponent", "Vector3 rotation", asOFFSET(TransformComponent, rotation)) < 0)
    {
        if(engine)engine->Log(LogLevel::Error, "TransformComponent のプロパティ rotation の登録に失敗しました");
    }
    if(scriptEngine_->RegisterObjectProperty("TransformComponent", "Vector3 scale", asOFFSET(TransformComponent, scale)) < 0)
    {
        if(engine)engine->Log(LogLevel::Error, "TransformComponent のプロパティ scale の登録に失敗しました");
    }

	// Log 関数の登録
    if(scriptEngine_->RegisterGlobalFunction("void Log(const string &in)",
        asFUNCTIONPR([](const std::string& str) 
            {
                Engine::GetInstance()->Log(LogLevel::Info, str);
            }, 
            (const std::string&), void), asCALL_CDECL) < 0)
    {
        if(engine)engine->Log(LogLevel::Error, "Log 関数の登録に失敗しました");
    }
}