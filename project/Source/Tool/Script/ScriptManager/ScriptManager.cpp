#include "ScriptManager.hpp"
#include "Engine.hpp"
#include <cassert>
#include <iostream>
#include <scriptarray/scriptarray.h>

#include "../ScriptRegisterAPI/ScriptRegisterAPI.hpp"

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

    // 標準アッドオンの登録 (std::vector サポート)
    RegisterScriptArray(scriptEngine_, true);

	// C++ エンジンAPIのバインド
	RegisterEngineAPI();

#ifdef DEVELOPMENT
    // VSCodeの予測変換用ファイルを自動生成
    ExportPredefinedFile("./as.predefined");
#endif
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
/// @param scriptClass 
/// @param scriptPath 
/// @return 
Detail::ScriptHandle Detail::ScriptManager::BuildScriptModule(const std::string& moduleName, const std::string& scriptClass, const std::string& scriptPath)
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
	slot.scriptClass = scriptClass;
	slot.lastWriteTime = asTime;

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

    // このスクリプトを参照している全エンティティの既存インスタンスを破棄する
    auto registry = Engine::GetInstance()->GetRegistryECS();
    if (registry)
    {
        for (Entity entity : registry->GetActiveEntities())
        {
            if (registry->HasComponent<ScriptComponent>(entity))
            {
                auto scriptComp = registry->GetComponent<ScriptComponent>(entity);
                if (scriptComp->handle.index == handle.index)
                {
                    // オブジェクトと関数ポインタをクリア
                    if (scriptComp->scriptObject)
                    {
                        scriptComp->scriptObject->Release();
                        scriptComp->scriptObject = nullptr;
                    }

                    scriptComp->initFunc = nullptr;
                    scriptComp->updateFunc = nullptr;
                    scriptComp->handle = ScriptHandle::Invalid();
                }
            }
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

/// @brief スクリプトハンドルからモジュール名を取得する
/// @param handle 
/// @return 
std::string Detail::ScriptManager::GetModuleName(ScriptHandle handle) const
{
	if (!handle.IsValid()) return "";

	// インデックスが範囲外、またはスロットが使用中でない場合は空文字を返す
	if (handle.index >= slots_.size() || !slots_[handle.index].inUse)
		return "";

	return slots_[handle.index].moduleName;
}

/// @brief スクリプトハンドルからスクリプトクラス名を取得する
/// @param handle 
/// @return 
std::string Detail::ScriptManager::GetScriptClass(ScriptHandle handle) const
{
	if (!handle.IsValid()) return "";

	// インデックスが範囲外、またはスロットが使用中でない場合は空文字を返す
	if (handle.index >= slots_.size() || !slots_[handle.index].inUse)
		return "";

	return slots_[handle.index].scriptClass;
}

/// @brief 変更されたスクリプトをリロードする
void Detail::ScriptManager::ReloadModifiedScripts()
{
	auto engine = Engine::GetInstance();
	auto registry = engine->GetRegistryECS();

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

				// 再ビルド前に、このスクリプトを参照している全エンティティの既存インスタンスを破棄する
				if (registry)
				{
					auto activeEntities = registry->GetActiveEntities();
					for (Entity entity : activeEntities)
					{
						if (registry->HasComponent<ScriptComponent>(entity))
						{
							auto scriptComp = registry->GetComponent<ScriptComponent>(entity);

							// 同じスクリプトハンドルを使っているかチェック
							if (scriptComp->handle.index == handle.index)
							{
								if (scriptComp->scriptObject)
								{
									// C++ 側で保持している参照カウントを解放
									scriptComp->scriptObject->Release();
									scriptComp->scriptObject = nullptr;
								}

								// 関数のポインタもリセット
								scriptComp->initFunc = nullptr;
								scriptComp->updateFunc = nullptr;
							}
						}
					}
				}

				// 再ビルド処理を呼び出す (内部でキャッシュスロットが上書き更新される)
				BuildScriptModule(slot.moduleName, slot.scriptClass, scriptPath);
			}
		}
	}
}

/// @brief エンティティからスクリプトオブジェクトを取得する
/// @param targetEntity 
/// @return 
asIScriptObject* Detail::ScriptManager::GetScriptFromEntity(Entity targetEntity)
{
	auto registry = Engine::GetInstance()->GetRegistryECS();

    // エンティティが有効で、かつScriptComponentを持っている場合にスクリプトオブジェクトを返す
	if (registry->HasComponent<ScriptComponent>(targetEntity))
	{
		return registry->GetComponent<ScriptComponent>(targetEntity)->scriptObject;
	}

	return nullptr;
}

/// @brief C++ エンジンAPIのバインド
void Detail::ScriptManager::RegisterEngineAPI()
{
    // 数学APIの登録
    ScriptRegisterAPI::RegisterMathAPI(scriptEngine_);

    // コンポーネントAPIの登録
    ScriptRegisterAPI::RegisterComponentAPI(scriptEngine_);

    // エンジンAPIの登録
    ScriptRegisterAPI::RegisterEngineAPI(scriptEngine_);

    // InputButton API の登録
    ScriptRegisterAPI::RegisterInputButtonAPI(scriptEngine_);

    // Input API の登録
    ScriptRegisterAPI::RegisterInputAPI(scriptEngine_);
}

/// @brief 事前定義されたスクリプトファイルをエクスポートする
/// @param outputPath 
void Detail::ScriptManager::ExportPredefinedFile(const std::string& outputPath)
{
    if (!scriptEngine_) return;

    std::ofstream ofs(outputPath);
    if (!ofs.is_open()) return;

    ofs << "// Auto-generated by ScriptManager for AngelScript LSP\n\n";

    // Enum の出力
    for (asUINT i = 0; i < scriptEngine_->GetEnumCount(); ++i)
    {
        asITypeInfo* enumType = scriptEngine_->GetEnumByIndex(i);
        std::string ns = enumType->GetNamespace();
        if (!ns.empty()) ofs << "namespace " << ns << " {\n";

        ofs << "enum " << enumType->GetName() << " {\n";
        for (asUINT j = 0; j < enumType->GetEnumValueCount(); ++j)
        {
            int value = 0;
            const char* name = enumType->GetEnumValueByIndex(j, &value);
            ofs << "    " << name << " = " << value << ",\n";
        }
        ofs << "}\n";

        if (!ns.empty()) ofs << "}\n";
        ofs << "\n";
    }

    // オブジェクト型・クラス・構造体の出力
    for (asUINT i = 0; i < scriptEngine_->GetObjectTypeCount(); ++i)
    {
        asITypeInfo* type = scriptEngine_->GetObjectTypeByIndex(i);
        std::string ns = type->GetNamespace();
        if (!ns.empty()) ofs << "namespace " << ns << " {\n";

        ofs << "class " << type->GetName() << " {\n";

        // コンストラクタ / ファクトリ
        for (asUINT j = 0; j < type->GetBehaviourCount(); ++j)
        {
            asEBehaviours beh;
            asIScriptFunction* func = type->GetBehaviourByIndex(j, &beh);
            if (beh == asBEHAVE_CONSTRUCT)
            {
                // "void f(...)" の形式を "ClassName(...)" に変換
                std::string decl = func->GetDeclaration(false, false, true);
                size_t pos = decl.find('(');
                if (pos != std::string::npos)
                {
                    ofs << "    " << type->GetName() << decl.substr(pos) << ";\n";
                }
            }
        }

        // プロパティ (x, y, z, position など)
        for (asUINT j = 0; j < type->GetPropertyCount(); ++j)
        {
            ofs << "    " << type->GetPropertyDeclaration(j) << ";\n";
        }

        // メソッド (Inverse, Transpose, Get, Set など)
        for (asUINT j = 0; j < type->GetMethodCount(); ++j)
        {
            asIScriptFunction* func = type->GetMethodByIndex(j);
            ofs << "    " << func->GetDeclaration(false, false, true) << ";\n";
        }

        ofs << "}\n";

        if (!ns.empty()) ofs << "}\n";
        ofs << "\n";
    }

    // グローバル関数の出力
    for (asUINT i = 0; i < scriptEngine_->GetGlobalFunctionCount(); ++i)
    {
        asIScriptFunction* func = scriptEngine_->GetGlobalFunctionByIndex(i);
        std::string ns = func->GetNamespace();
        if (!ns.empty()) ofs << "namespace " << ns << " {\n    ";

        ofs << func->GetDeclaration(false, false, true) << ";\n";

        if (!ns.empty()) ofs << "}\n";
    }

    // グローバルプロパティの出力
    for (asUINT i = 0; i < scriptEngine_->GetGlobalPropertyCount(); ++i)
    {
        const char* name = nullptr;
        const char* ns = nullptr;
        int typeId = 0;
        bool isConst = false;
        scriptEngine_->GetGlobalPropertyByIndex(i, &name, &ns, &typeId, &isConst);

        std::string nsStr = ns ? ns : "";
        if (!nsStr.empty()) ofs << "namespace " << nsStr << " {\n    ";

        if (isConst) ofs << "const ";
        ofs << scriptEngine_->GetTypeDeclaration(typeId) << " " << name << ";\n";

        if (!nsStr.empty()) ofs << "}\n";
    }

    ofs.close();
}
