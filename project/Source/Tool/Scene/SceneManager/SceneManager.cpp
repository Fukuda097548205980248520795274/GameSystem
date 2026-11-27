#include "SceneManager.hpp"
#include "Engine.hpp"

/// @brief 初期化
/// @param name 
void Detail::SceneManager::Initialize(const std::string& name)
{
    if (!name.empty())
    {
        // 初期化時に指定されたシーン名を読み込む
        LoadSceneInternal(name);
    }
}

/// @brief シーンをリストから削除する
/// @param name 
void Detail::SceneManager::RemoveScene(const std::string& name)
{
    scenes_.erase(std::remove_if(scenes_.begin(), scenes_.end(),
        [&name](const SceneInfo& scene) { return scene.name == name; }), scenes_.end());
}

/// @brief 現在のECS状態を指定したシーンファイルに保存する
bool Detail::SceneManager::SaveCurrentScene()
{
    if (currentSceneName_.empty()) return false;

    // 現在のECS状態をJSON形式でシリアライズして保存
    bool result = Detail::ToJson(Engine::GetInstance()->GetRegistryECS(), currentSceneName_);
    if (result)
    {
        isDirty_ = false;
    }
    
    return result;
}

/// @brief 更新処理
void Detail::SceneManager::Update()
{
    if (!nextSceneName_.empty())
    {
        // 次のシーンが指定されている場合、シーンを切り替える
        LoadSceneInternal(nextSceneName_);
        nextSceneName_.clear();
    }
}

/// @brief ファイルパスを取得する
/// @param name 
/// @return 
std::string Detail::SceneManager::GetFilePath(const std::string& name) const
{
    for (const auto& scene : scenes_)
    {
        if (scene.name == name) return scene.filePath;
    }

    return "";
}

/// @brief シーンを内部的に読み込む
/// @param name 
/// @return 
bool Detail::SceneManager::LoadSceneInternal(const std::string& name)
{
    auto engine = Engine::GetInstance();
    if (!engine) return false;

    auto registry = engine->GetRegistryECS();
    if (!registry) return false;

    // 現在のエンティティを完全にクリア
    registry->Clear();

    // 基本は bin (バイナリ) ファイルからの高速な読み込みを試みる
    bool isLoaded = Detail::LoadBin(registry, name);

    // bin が無い、または読み込みに失敗した場合は json からの読み込みを試みる（フォールバック）
    if (!isLoaded)
    {
        // JSONからの読み込みを試みる
        isLoaded = Detail::LoadJson(registry, name);

        // JSONからの読み込みに成功した場合、binファイルを生成しておく
        if (isLoaded)
        {
            if (Detail::ToJson(registry, name))
            {
                engine->Log(LogLevel::Info, "シーンのbinファイルを自動生成しました : " + name);
            }
            else
            {
                engine->Log(LogLevel::Warning, "シーンのbinファイル自動生成に失敗しました : " + name);
            }
        }
    }

    // 読み込みに成功したら現在のシーン名を更新
    if (isLoaded)
    {
        currentSceneName_ = name;
        isDirty_ = false;
    }
    else
    {
        engine->Log(LogLevel::Error, "シーンの読み込みに失敗しました : " + name);
    }

    return isLoaded;
}
