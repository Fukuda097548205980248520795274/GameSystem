#pragma once
#include "../SceneManager/SceneManager.hpp"
#include <memory>

namespace Detail
{
    class SceneManager;

    class SceneEditor
    {
    public:

        // @brief コンストラクタ
        SceneEditor() {Initialize();}

        // @brief デストラクタ
        ~SceneEditor() = default;

        /// @brief 新規シーンを作成する
        /// @param sceneName 作成するシーンの名前
        void CreateNewScene(const std::string& sceneName);

        /// @brief 現在のシーンを保存する
        void SaveCurrentScene();

        /// @brief シーンを読み込む
        /// @param sceneName 読み込むシーンの名前
        void LoadScene(const std::string& sceneName);

        /// @brief 更新処理
        void Update() { sceneManager_->Update(); }

        /// @brief エディタのUIを描画する (ImGui用)
        void DrawUI();

    private:

        /// @brief エディタの初期化
        void Initialize();

        // シーンマネージャー
        std::unique_ptr<SceneManager> sceneManager_;

        // プロジェクト設定ファイルのパス（シーンリスト保存用）
        const std::string kProjectSettingsPath = "./Assets/Data/ProjectSettings.json";

        // プロジェクト設定のバイナリパス（シーンリスト保存用）
        const std::string kProjectSettingsBinPath = "./Cooked/Data/ProjectSettings.bin";
    };
}
