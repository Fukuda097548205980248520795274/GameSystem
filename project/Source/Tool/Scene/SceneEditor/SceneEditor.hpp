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

        /// @brief シーンを削除する
        /// @param sceneName 削除するシーンの名前
        void DeleteScene(const std::string& sceneName);

        /// @brief 現在のシーンを保存する
        void SaveCurrentScene();

        /// @brief シーンを読み込む
        /// @param sceneName 読み込むシーンの名前
        void LoadScene(const std::string& sceneName);

        /// @brief 更新処理
        void Update() { sceneManager_->Update(); }

        /// @brief エディタのUIを描画する (ImGui用)
        void DrawUI();

        /// @brief シーンマネージャを取得する
        /// @return 
        SceneManager* GetSceneManager() const { return sceneManager_.get(); }


    private:

        /// @brief エディタの初期化
        void Initialize();

        // シーンマネージャー
        std::unique_ptr<SceneManager> sceneManager_;

        // プロジェクト設定ファイルのパス（シーンリスト保存用）
        const std::string kProjectSettingsPath = "./Assets/Data/ProjectSettings.json";

        // プロジェクト設定のバイナリパス（シーンリスト保存用）
        const std::string kProjectSettingsBinPath = "./Cooked/Data/ProjectSettings.bin";


    private:

        /// @brief 保存確認後に実行する保留中のアクション
        enum class PendingSceneAction
        {
            None,
            LoadScene,
            CreateNewScene
        };

        // 保留中のシーンアクション
        PendingSceneAction pendingAction_ = PendingSceneAction::None;

        // 保留中のシーン名（保存確認後に使用する）
        std::string pendingSceneName_;

        // @brief 保存確認モーダルが開いているかどうか
        bool isOpenSaveConfirmModal_ = false;

        // @brief 新規シーン作成モーダルが開いているかどうか
        bool isOpenNewSceneModal_ = false;
    };
}
