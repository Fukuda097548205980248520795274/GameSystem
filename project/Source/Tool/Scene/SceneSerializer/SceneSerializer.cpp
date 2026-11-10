#include "SceneSerializer.hpp"
#include <fstream>
#include "Scene/SceneManager/SceneManager.hpp"

/// @brief SceneManagerに登録されているシーン一覧をJSONとして保存する
/// @param sceneManager 
/// @param filePath 
/// @param binPath
/// @return 
bool Detail::SerializeSceneList(const SceneManager* sceneManager, const std::string& jsonPath, const std::string& binPath)
{
    if (!sceneManager) return false;

    nlohmann::json rootJson;
    rootJson["scenes"] = nlohmann::json::array();

    // SceneManagerからシーン一覧を取得して配列に格納
    for (const auto& scene : sceneManager->GetSceneList())
    {
        nlohmann::json sceneJson;
        sceneJson["name"] = scene.name;
        sceneJson["filePath"] = scene.filePath;
        rootJson["scenes"].push_back(sceneJson);
    }

    // JSONファイルに書き出し
    std::ofstream jsonFile(jsonPath);
    if (jsonFile.is_open())
    {
        jsonFile << rootJson.dump(4);
    }

    // BIN(MessagePack)ファイルに書き出し
    std::ofstream binFile(binPath, std::ios::binary);
    if (binFile.is_open())
    {
        std::vector<uint8_t> binaryData = nlohmann::json::to_msgpack(rootJson);
        binFile.write(reinterpret_cast<const char*>(binaryData.data()), binaryData.size());
    }

    return true;
}

/// @brief JSONからシーン一覧を読み込み、SceneManagerに登録する
/// @param sceneManager 
/// @param jsonPath 
/// @param binPath
/// @return 
bool Detail::DeserializeSceneList(SceneManager* sceneManager, const std::string& jsonPath, const std::string& binPath)
{
    if (!sceneManager) return false;

    nlohmann::json rootJson;
    bool isLoaded = false;

    // BINファイルからの読み込みを試みる
    std::ifstream binFile(binPath, std::ios::binary);
    if (binFile.is_open())
    {
        std::vector<uint8_t> binaryData((std::istreambuf_iterator<char>(binFile)), std::istreambuf_iterator<char>());
        rootJson = nlohmann::json::from_msgpack(binaryData); // バイナリをJSONに変換
        isLoaded = true;
    }
    else
    {
        // BINファイルがない場合、JSONファイルからそのまま読み込む
        std::ifstream jsonFile(jsonPath);
        if (jsonFile.is_open())
        {
            jsonFile >> rootJson;
            isLoaded = true;
        }
    }

    if (!isLoaded) return false;

    // JSONオブジェクトからシーンデータを復元
    if (rootJson.contains("scenes"))
    {
        for (const auto& sceneJson : rootJson["scenes"])
        {
            std::string name = sceneJson["name"];
            std::string path = sceneJson["filePath"];

            // SceneManagerにシーンを復元登録
            sceneManager->AddScene(name, path);
        }
    }

    return true;
}
