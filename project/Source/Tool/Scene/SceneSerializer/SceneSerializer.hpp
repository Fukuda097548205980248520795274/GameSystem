#pragma once
#include <json.hpp>

namespace Detail
{
    class SceneManager;

    /// @brief SceneManagerに登録されているシーン一覧をJSONとして保存する
    /// @param sceneManager 
    /// @param jsonPath 
    /// @param binPath
    /// @return 
    bool SerializeSceneList(const SceneManager* sceneManager, const std::string& jsonPath, const std::string& binPath);

    /// @brief JSONからシーン一覧を読み込み、SceneManagerに登録する
    /// @param sceneManager 
    /// @param jsonPath 
    /// @param binPath
    /// @return 
    bool DeserializeSceneList(SceneManager* sceneManager, const std::string& jsonPath, const std::string& binPath);
}
