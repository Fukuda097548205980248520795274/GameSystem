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

        /// @brief 初期化
        /// @param name 
        void Initialize(const std::string& name);

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
        bool SaveCurrentScene();

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

        /// @brief シーンに変更があったことを記録する
        void MarkDirty() { isDirty_ = true; }

        /// @brief 未保存の変更があるかを取得する
        /// @return 
        bool IsDirty() const { return isDirty_; }

        /// @brief 変更フラグをリセットする
        void ClearDirty() { isDirty_ = false; }


    private:

        /// @brief ファイルパスを取得する
        /// @param name 
        /// @return 
        std::string GetFilePath(const std::string& name) const;

        /// @brief シーンを内部的に読み込む
        /// @param name 
        /// @return 
        bool LoadSceneInternal(const std::string& name);


    private:

        // シーンのリスト
        std::vector<SceneInfo> scenes_;

        // 現在のシーンの名前
        std::string currentSceneName_;

        // 次に読み込むシーンの名前
        std::string nextSceneName_;

        // 起動時に読み込むシーンの名前
        std::string startupSceneName_;

        // シーンに変更があったかどうかのフラグ
        bool isDirty_ = false;
    };
}
