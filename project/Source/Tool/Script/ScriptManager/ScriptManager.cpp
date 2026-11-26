#include "ScriptManager.hpp"
#include "Engine.hpp"
#include <cassert>
#include <iostream>
#include <unordered_set>
#include <scriptarray/scriptarray.h>

#include "../ScriptRegisterAPI/ScriptRegisterAPI.hpp"

namespace
{
    /// @brief メインファイルとインクルードファイル群の中で最新の更新日時を取得する
    /// @param mainPath 
    /// @param includedFiles 
    /// @param outLatestTime 
    /// @return 
    bool TryGetLatestWriteTime(const std::string& mainPath,const std::vector<std::string>& includedFiles,std::filesystem::file_time_type& outLatestTime)
    {
        // メインファイルが存在しない場合は失敗
        if (!std::filesystem::exists(mainPath)) return false;
        outLatestTime = std::filesystem::last_write_time(mainPath);

        // インクルードファイル群の中で最新の更新日時を取得
        for (const auto& incPath : includedFiles)
        {
            if (!std::filesystem::exists(incPath)) return false;
            auto incTime = std::filesystem::last_write_time(incPath);

            // 最新の更新日時を更新
            if (incTime > outLatestTime)
            {
                outLatestTime = incTime;
            }
        }

        return true;
    }
}

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

    // フォルダ内のスクリプトのメタデータ（パスやクラス名）のみを登録する
    ScanScriptDirectory();

    // VSCodeの予測変換用ファイルを自動生成
    ExportPredefinedFile("./as.predefined");

#endif
}

/// @brief シャットダウン
void Detail::ScriptManager::Shutdown()
{
    // 先にすべてのスクリプトオブジェクトを解放する
    if (auto engine = Engine::GetInstance())
    {
        if (auto registry = engine->GetRegistryECS())
        {
            auto activeEntities = registry->GetActiveEntities();

            for (Entity entity : activeEntities)
            {
                if (registry->HasComponent<ScriptComponent>(entity))
                {
                    auto scriptComp = registry->GetComponent<ScriptComponent>(entity);
                    if (scriptComp->scriptObject)
                    {
                        scriptComp->scriptObject->Release();
                        scriptComp->scriptObject = nullptr;
                    }

                    scriptComp->initFunc = nullptr;
                    scriptComp->updateFunc = nullptr;
                }
            }
        }
    }

    // すべてのオブジェクト解放後にエンジン本体をシャットダウン・解放する
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

    // メモリキャッシュチェック（インクルードファイルも含めて確認）
    auto it = cache_.find(scriptPath);
    if (it != cache_.end())
    {
        ScriptHandle cachedHandle = GetHandleByIndex(it->second.index);
        if (cachedHandle != ScriptHandle::Invalid())
        {
            const Slot& cachedSlot = slots_[cachedHandle.index];
            std::filesystem::file_time_type latestTime;
            if (cachedSlot.isLoaded &&
                TryGetLatestWriteTime(scriptPath, cachedSlot.includedFiles, latestTime) &&
                cachedSlot.lastWriteTime >= latestTime)
            {
                return cachedHandle;
            }
        }
    }

    std::string binPath = kCookedDir + asFilePath.stem().string() + ".bin";
    if (!std::filesystem::exists(kCookedDir))
        std::filesystem::create_directories(kCookedDir);

    bool isLoaded = false;
    std::vector<std::string> includedFiles;

    // .binからのロード試行
    if (std::filesystem::exists(binPath))
    {
        auto binTime = std::filesystem::last_write_time(binPath);
        CBytecodeStream stream(binPath, false);
        uint32_t magic = 0;
        std::string dummyModule, dummyClass, dummyPath;
        uint32_t includeCount = 0;

        bool headerOk = stream.IsOpen() &&
            stream.ReadUint32(magic) && magic == kBinMagic &&
            stream.ReadString(dummyModule) &&
            stream.ReadString(dummyClass) &&
            stream.ReadString(dummyPath) &&
            stream.ReadUint32(includeCount);

        if (headerOk)
        {
            std::vector<std::string> binIncludes;
            binIncludes.reserve(includeCount);
            for (uint32_t i = 0; i < includeCount; ++i)
            {
                std::string incPath;
                if (!stream.ReadString(incPath))
                {
                    headerOk = false;
                    break;
                }
                binIncludes.push_back(std::move(incPath));
            }

            // メインファイルと全インクルードファイルが .bin より古いかチェック
            std::filesystem::file_time_type latestTime;
            if (headerOk &&
                TryGetLatestWriteTime(scriptPath, binIncludes, latestTime) &&
                binTime >= latestTime)
            {
                asIScriptModule* mod = scriptEngine_->GetModule(moduleName.c_str(), asGM_ALWAYS_CREATE);
                if (mod && mod->LoadByteCode(&stream) >= 0)
                {
                    isLoaded = true;
                    includedFiles = std::move(binIncludes);
                } else if (engine)
                {
                    engine->Log(LogLevel::Warning, "バイトコードの読み込みに失敗したため再ビルドします : " + binPath);
                }
            }
        }
    }

    if (engine)
        engine->Log(LogLevel::Info, "スクリプトモジュールのビルド : " + moduleName + (isLoaded ? " (バイトコードからロード)" : " (ソースからビルド)"));

    // ソースからビルド
    if (!isLoaded)
    {
        includedFiles.clear();

        auto includeCallback = [](const char* include, const char* from, CScriptBuilder* builder, void* userParam) -> int
            {
                std::filesystem::path baseDir = std::filesystem::path(from).parent_path();
                std::filesystem::path targetPath = (baseDir / include).lexically_normal();

                std::ifstream file(targetPath, std::ios::binary);
                if (!file.is_open()) return -1;

                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string code = buffer.str();

                std::string normalizedPath = targetPath.generic_string();

                // インクルードしたファイルパスを記録
                if (userParam)
                {
                    auto* list = static_cast<std::vector<std::string>*>(userParam);
                    list->push_back(normalizedPath);
                }

                return builder->AddSectionFromMemory(normalizedPath.c_str(), code.c_str(), static_cast<uint32_t>(code.length()));
            };

        CScriptBuilder builder;
        // userParam に includedFiles のポインタを渡す
        builder.SetIncludeCallback(includeCallback, &includedFiles);

        int r = builder.StartNewModule(scriptEngine_, moduleName.c_str());
        if (r < 0)
        {
            if (engine) engine->StopExecution();
            return ScriptHandle::Invalid();
        }

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

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string scriptCode = buffer.str();
        file.close();

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
        if (engine) engine->Log(LogLevel::Info, "スクリプトモジュールをビルド : " + moduleName);

        // バイトコードとインクルード一覧を保存
        asIScriptModule* mod = builder.GetModule();
        if (mod)
        {
            CBytecodeStream stream(binPath, true);
            if (stream.IsOpen())
            {
                stream.WriteUint32(kBinMagic);
                stream.WriteString(moduleName);
                stream.WriteString(scriptClass);
                stream.WriteString(scriptPath);
                stream.WriteUint32(static_cast<uint32_t>(includedFiles.size()));
                for (const auto& incPath : includedFiles)
                {
                    stream.WriteString(incPath);
                }
                mod->SaveByteCode(&stream);
            }
        }
    }

    // スロットの割り当て（既存処理と同じ）
    uint32_t slotIndex = 0;
    if (it != cache_.end() && GetHandleByIndex(it->second.index) != ScriptHandle::Invalid())
    {
        slotIndex = it->second.index;
    } else if (!freeSlots_.empty())
    {
        slotIndex = freeSlots_.front();
        freeSlots_.pop();
    } else
    {
        slotIndex = static_cast<uint32_t>(slots_.size());
        slots_.emplace_back();
    }

    std::filesystem::file_time_type latestTime;
    TryGetLatestWriteTime(scriptPath, includedFiles, latestTime);

    Slot& slot = slots_[slotIndex];
    slot.inUse = true;
    slot.isLoaded = true;
    slot.filePath = scriptPath;
    slot.moduleName = moduleName;
    slot.scriptClass = scriptClass;
    slot.includedFiles = std::move(includedFiles);
    slot.lastWriteTime = latestTime;

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

    // 未使用、または世代番号が一致しない（古いハンドル）場合は何もしない
    if (!slot.inUse || slot.generation != handle.generation) return;

    // スクリプトモジュールを破棄
    if (scriptEngine_ && !slot.moduleName.empty())
    {
        scriptEngine_->DiscardModule(slot.moduleName.c_str());
    }

    // スロットを解放し、世代番号を進める
    slot.inUse = false;
    slot.isLoaded = false;
    slot.generation++;
    freeSlots_.push(handle.index);

    // キャッシュから該当するスクリプトパスを削除
    for (auto it = cache_.begin(); it != cache_.end(); )
    {
        if (it->second == handle)
        {
            it = cache_.erase(it);
        } else
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
                if (scriptComp->handle == handle)
                {
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
    if (!handle.IsValid() || handle.index >= slots_.size() || !slots_[handle.index].inUse)
        return "";

    // 世代番号が一致しない場合は空文字を返す
    const Slot& slot = slots_[handle.index];
    if (slot.generation != handle.generation)return "";

    return slot.filePath;
}

/// @brief スクリプトハンドルからモジュール名を取得する
/// @param handle 
/// @return 
std::string Detail::ScriptManager::GetModuleName(ScriptHandle handle) const
{
    if (!handle.IsValid() || handle.index >= slots_.size()) return "";

    // スロットが使用中でない場合は空文字を返す
    const Slot& slot = slots_[handle.index];
    if (!slot.inUse || slot.generation != handle.generation) return "";

    return slot.moduleName;
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

    // 世代番号が一致しない場合も空文字を返す
    const Slot& slot = slots_[handle.index];
    if (slot.generation != handle.generation) return "";

	return slot.scriptClass;
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
        if (!slot.inUse || !slot.isLoaded) continue;

        // メインファイルと全インクルードファイルの最新更新日時を取得
        std::filesystem::file_time_type latestWriteTime;
        if (!TryGetLatestWriteTime(scriptPath, slot.includedFiles, latestWriteTime)) continue;

        // いずれかのファイルがキャッシュ時点より新しければ再ビルド
        if (latestWriteTime > slot.lastWriteTime)
        {
            if (engine) engine->Log(LogLevel::Info, "スクリプトの更新を検知しました。再ビルドします : " + scriptPath);

            if (registry)
            {
                auto activeEntities = registry->GetActiveEntities();
                for (Entity entity : activeEntities)
                {
                    if (registry->HasComponent<ScriptComponent>(entity))
                    {
                        auto scriptComp = registry->GetComponent<ScriptComponent>(entity);
                        if (scriptComp->handle.index == handle.index)
                        {
                            if (scriptComp->scriptObject)
                            {
                                scriptComp->scriptObject->Release();
                                scriptComp->scriptObject = nullptr;
                            }
                            scriptComp->initFunc = nullptr;
                            scriptComp->updateFunc = nullptr;
                        }
                    }
                }
            }

            BuildScriptModule(slot.moduleName, slot.scriptClass, scriptPath);
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

/// @brief スクリプトのメタデータのみを登録する（実データはロードしない）
/// @param moduleName 
/// @param scriptClass 
/// @param scriptPath 
/// @return 
Detail::ScriptHandle Detail::ScriptManager::RegisterScriptMetadata(const std::string& moduleName, const std::string& scriptClass, const std::string& scriptPath)
{
    // すでに登録済みなら既存のハンドルを返す
    auto it = cache_.find(scriptPath);
    if (it != cache_.end())
    {
        ScriptHandle cachedHandle = GetHandleByIndex(it->second.index);
        if (cachedHandle != ScriptHandle::Invalid())
        {
            return cachedHandle;
        }
    }

    // スロットの割り当て
    uint32_t slotIndex = 0;
    if (!freeSlots_.empty())
    {
        slotIndex = freeSlots_.front();
        freeSlots_.pop();
    }
    else
    {
        slotIndex = static_cast<uint32_t>(slots_.size());
        slots_.emplace_back();
    }

    Slot& slot = slots_[slotIndex];
    slot.inUse = true;
    slot.isLoaded = false; // 未ロード状態
    slot.filePath = scriptPath;
    slot.moduleName = moduleName;
    slot.scriptClass = scriptClass;

    ScriptHandle handle{ slotIndex, slot.generation };
    cache_[scriptPath] = handle;

    return handle;
}

/// @brief 指定ハンドルのスクリプトが未ロードであればロードする（遅延ロード）
/// @param handle 
/// @return 
bool Detail::ScriptManager::EnsureLoaded(ScriptHandle handle)
{
    if (!handle.IsValid() || handle.index >= slots_.size()) return false;

    // スロットの有効性を確認
    Slot& slot = slots_[handle.index];
    if (!slot.inUse || slot.generation != handle.generation) return false;

    // すでにロード済みなら何もしない
    if (slot.isLoaded) return true;

    // スクリプトモジュールをビルドしてロードする
    ScriptHandle builtHandle = BuildScriptModule(slot.moduleName, slot.scriptClass, slot.filePath);
    return builtHandle.IsValid();
}

/// @brief スクリプトがロード済みか確認する
/// @param handle 
/// @return 
bool Detail::ScriptManager::IsLoaded(ScriptHandle handle) const
{
    if (!handle.IsValid() || handle.index >= slots_.size()) return false;

    // スロットの有効性を確認
    const Slot& slot = slots_[handle.index];
    return slot.inUse && slot.generation == handle.generation && slot.isLoaded;
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

/// @brief フォルダを走査してスクリプトのメタデータのみを登録する
void Detail::ScriptManager::ScanScriptDirectory()
{
    if (!std::filesystem::exists(kScriptDir)) return;

    // スクリプトディレクトリを再帰的に走査して、.as ファイルを見つける
    for (const auto& entry : std::filesystem::recursive_directory_iterator(kScriptDir))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".as") continue;

        std::string fileName = entry.path().stem().string();
        std::string scriptPath = entry.path().generic_string();

        // メタデータだけ登録（ファイルの中身はまだ開かない）
        RegisterScriptMetadata(fileName, fileName, scriptPath);
    }
}
