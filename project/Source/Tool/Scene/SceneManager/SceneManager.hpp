#pragma once
#include <string>
#include <vector>

namespace Detail
{
    /// @brief シーンのメタデータ
    struct SceneInfo
    {
        // シーンの名前
        std::string name;

        // シーンのファイルパス
        std::string filePath;
    };

    /// @brief シーンマネージャー
    class SceneManager
    {
    public:

        // @brief コンストラクタ
        SceneManager() = default;

        // @brief デストラクタ
        ~SceneManager() = default;

        /// @brief 新しい空のシーンをリストに追加する
        /// @param name 
        /// @param filePath 
        void AddScene(const std::string& name, const std::string& filePath) { scenes_.push_back({ name, filePath }); }

        /// @brief シーンをリストから削除する
        /// @param name 
        void RemoveScene(const std::string& name);

        /// @brief 登録されているシーン一覧を取得
        /// @return 
        const std::vector<SceneInfo>& GetSceneList() const { return scenes_; }

        /// @brief シーンを読み込む
        /// @param name 
        void LoadScene(const std::string& name) { nextSceneName_ = name; }

        /// @brief 現在のECS状態を指定したシーンファイルに保存する
        void SaveCurrentScene();

        /// @brief 更新処理
        void Update();

        /// @brief 現在のシーンの名前を取得
        /// @return 
        const std::string& GetCurrentSceneName() const { return currentSceneName_; }

        /// @brief 現在のシーンの名前を設定する
        /// @param name 
        void SetCurrentSceneName(const std::string& name) { currentSceneName_ = name; }

        /// @brief 起動時のシーンの名前を取得する
        /// @return 
        const std::string& GetStartupSceneName() const { return startupSceneName_; }

        /// @brief 起動時のシーンの名前を設定する
        /// @param name 
        void SetStartupSceneName(const std::string& name) { startupSceneName_ = name; }


    private:

        /// @brief ファイルパスを取得する
        /// @param name 
        /// @return 
        std::string GetFilePath(const std::string& name) const;


    private:

        // シーンのリスト
        std::vector<SceneInfo> scenes_;

        // 現在のシーンの名前
        std::string currentSceneName_;

        // 次に読み込むシーンの名前
        std::string nextSceneName_;

        // 起動時に読み込むシーンの名前
        std::string startupSceneName_;
    };
}
