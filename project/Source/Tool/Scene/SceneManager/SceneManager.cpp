#include "SceneManager.hpp"
#include "Engine.hpp"

/// @brief 初期化
/// @param name 
void Detail::SceneManager::Initialize(const std::string& name)
{
    if (!name.empty())
    {
        auto engine = Engine::GetInstance();
        if (!engine) return;

        auto registry = engine->GetRegistryECS();
        if (!registry) return;

        // 現在のエンティティを完全にクリア
        registry->Clear();

        // 基本は bin (バイナリ) ファイルからの高速な読み込みを試みる
        bool isLoaded = Detail::LoadBin(registry, name);

        // bin が無い、または読み込みに失敗した場合は json からの読み込みを試みる（フォールバック）
        if (!isLoaded)
        {
            isLoaded = Detail::LoadJson(registry, name);
        }

        // 読み込みに成功したら現在のシーン名を更新
        if (isLoaded)
        {
            currentSceneName_ = name;
        }
        else
        {
            // ログ出力などで読み込み失敗を通知すると親切です
            engine->Log(LogLevel::Error, "シーンの読み込みに失敗しました : " + name);
        }
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
void Detail::SceneManager::SaveCurrentScene()
{
    if (currentSceneName_.empty()) return;

    // 現在のECS状態をJSON形式でシリアライズして保存
    Detail::ToJson(Engine::GetInstance()->GetRegistryECS(), currentSceneName_);
}

/// @brief 更新処理
void Detail::SceneManager::Update()
{
    if (!nextSceneName_.empty())
    {
        auto engine = Engine::GetInstance();
        auto registry = engine->GetRegistryECS();

        // 現在のエンティティを完全にクリア
        registry->Clear();

        // 基本は bin (バイナリ) ファイルからの高速な読み込みを試みる
        bool isLoaded = Detail::LoadBin(registry, nextSceneName_);

        // bin が無い、または読み込みに失敗した場合は json からの読み込みを試みる（フォールバック）
        if (!isLoaded)
        {
            isLoaded = Detail::LoadJson(registry, nextSceneName_);
        }

        // 読み込みに成功したら現在のシーン名を更新
        if (isLoaded)
        {
            currentSceneName_ = nextSceneName_;
        }
        else
        {
            // ログ出力などで読み込み失敗を通知すると親切です
            engine->Log(LogLevel::Error, "シーンの読み込みに失敗しました : " + nextSceneName_);
        }

        nextSceneName_ = "";
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
