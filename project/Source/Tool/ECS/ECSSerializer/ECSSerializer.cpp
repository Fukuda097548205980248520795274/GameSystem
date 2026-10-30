#include "ECSSerializer.hpp"
#include <filesystem>
#include <fstream>
#include "ECS/RegistryECS/RegistryECS.hpp"
#include "ECS/ComponentECS/ComponentECS.hpp"

/// @brief json形式にシリアライズする
/// @param registry 
/// @param sceneName 
/// @return 
bool Detail::ToJson(RegistryECS* registry, const std::string& sceneName)
{
    if (!registry) return false;

	// jsonファイルのパスを作成
    nlohmann::json rootJson;
    rootJson["entities"] = nlohmann::json::array();

    // 有効なすべてのエンティティをシリアライズ
    const auto& activeEntities = registry->GetActiveEntities();
    for (Entity entity : activeEntities)
    {
        nlohmann::json entityJson;
        entityJson["id"] = entity;
        entityJson["components"] = nlohmann::json::object();

        // TransformComponent
        if (registry->HasComponent<TransformComponent>(entity))
        {
            auto* c = registry->GetComponent<TransformComponent>(entity);
            entityJson["components"]["TransformComponent"] = {
                {"position", {c->position.x, c->position.y, c->position.z}},
                {"rotation", {c->rotation.x, c->rotation.y, c->rotation.z}},
                {"scale",    {c->scale.x,    c->scale.y,    c->scale.z}}
            };
        }

        // ScriptComponent
        if (registry->HasComponent<ScriptComponent>(entity))
        {
            auto* c = registry->GetComponent<ScriptComponent>(entity);
            entityJson["components"]["ScriptComponent"] = {
                {"moduleName", c->moduleName},
                {"scriptClass", c->scriptClass}
            };
        }

        // MeshComponent
        if (registry->HasComponent<MeshComponent>(entity))
        {
            auto* c = registry->GetComponent<MeshComponent>(entity);
            entityJson["components"]["MeshComponent"] = {
                {"meshType", static_cast<int>(c->meshType)}
            };
        }

        // MaterialComponent
        if (registry->HasComponent<MaterialComponent>(entity))
        {
            auto* c = registry->GetComponent<MaterialComponent>(entity);
            entityJson["components"]["MaterialComponent"] = {
                {"psoName", c->psoName},
                {"color", {c->color.x, c->color.y, c->color.z, c->color.w}}
            };
        }

        // HierarchyComponent
        if (registry->HasComponent<HierarchyComponent>(entity))
        {
            auto* c = registry->GetComponent<HierarchyComponent>(entity);
            entityJson["components"]["HierarchyComponent"] = {
                {"parent", c->parent},
                {"children", c->children}
            };
        }

        rootJson["entities"].push_back(entityJson);
    }

    // 保存先ディレクトリの作成確認
    std::filesystem::create_directories("./Assets/Data/ECS");
    std::filesystem::create_directories("./Cooked/Data/ECS");

	// JSONファイルとして保存
    std::string jsonPath = "./Assets/Data/ECS/" + sceneName + ".json";
    std::ofstream jsonFile(jsonPath);
    if (jsonFile.is_open())
    {
        jsonFile << rootJson.dump(4); // インデント付きで保存
        jsonFile.close();
    }

	// バイナリファイルとして保存
    std::string binPath = "./Cooked/Data/ECS/" + sceneName + ".bin";
    std::ofstream binFile(binPath, std::ios::binary);
    if (binFile.is_open())
    {
        // MessagePack形式でバイナリデータ化
        std::vector<uint8_t> binaryData = nlohmann::json::to_msgpack(rootJson);
        binFile.write(reinterpret_cast<const char*>(binaryData.data()), binaryData.size());
        binFile.close();
    }

    return true;
}

/// @brief jsonファイルからロードする
/// @param registry 
/// @param sceneName 
/// @return 
bool Detail::LoadJson(RegistryECS* registry, const std::string& sceneName)
{
	// jsonファイルのパスを作成
    std::string jsonPath = "./Assets/Data/ECS/" + sceneName + ".json";
    std::ifstream jsonFile(jsonPath);
    if (!jsonFile.is_open()) return false;

	// JSONファイルを読み込む
    nlohmann::json rootJson;
    jsonFile >> rootJson;
    jsonFile.close();
    
	return Deserialize(registry, rootJson);
}

/// @brief バイナリファイルからロードする
/// @param registry 
/// @param sceneName 
/// @return 
bool Detail::LoadBin(RegistryECS* registry, const std::string& sceneName)
{
	// バイナリファイルのパスを作成
    std::string binPath = "./Cooked/Data/ECS/" + sceneName + ".bin";
    std::ifstream binFile(binPath, std::ios::binary);
    if (!binFile.is_open()) return false;

	// バイナリファイルを読み込む
    std::vector<uint8_t> binaryData((std::istreambuf_iterator<char>(binFile)), std::istreambuf_iterator<char>());
    binFile.close();

	// MessagePack形式のバイナリデータをJSONに変換
    nlohmann::json rootJson = nlohmann::json::from_msgpack(binaryData);

    return Deserialize(registry, rootJson);
}

/// @brief jsonデータからのエンティティ／コンポーネント再構築処理
/// @param registry 
/// @param rootJson 
/// @return 
bool Detail::Deserialize(RegistryECS* registry, const nlohmann::json& rootJson)
{
    if (!rootJson.contains("entities")) return false;

    // 古いエンティティIDから新しいエンティティIDへの対応表
    std::unordered_map<Entity, Entity> idMap;

    // エンティティの事前生成とIDマッピングの作成
    for (const auto& entityJson : rootJson["entities"])
    {
        Entity oldId = entityJson["id"];
        Entity newId = registry->CreateEntity();
        idMap[oldId] = newId;
    }

    // コンポーネントの復元
    for (const auto& entityJson : rootJson["entities"])
    {
        Entity newId = idMap[entityJson["id"]];
        const auto& componentsJson = entityJson["components"];

        // TransformComponent の復元
        if (componentsJson.contains("TransformComponent"))
        {
            TransformComponent comp;
            const auto& cJson = componentsJson["TransformComponent"];
            comp.position = Vector3(cJson["position"][0], cJson["position"][1], cJson["position"][2]);
            comp.rotation = Vector3(cJson["rotation"][0], cJson["rotation"][1], cJson["rotation"][2]);
            comp.scale = Vector3(cJson["scale"][0], cJson["scale"][1], cJson["scale"][2]);
            registry->AddComponent<TransformComponent>(newId, comp);
        }

        // ScriptComponent の復元
        if (componentsJson.contains("ScriptComponent"))
        {
            ScriptComponent comp;
            comp.moduleName = componentsJson["ScriptComponent"]["moduleName"];
            comp.scriptClass = componentsJson["ScriptComponent"]["scriptClass"];
            registry->AddComponent<ScriptComponent>(newId, comp);
        }

        // MaterialComponent の復元
        if (componentsJson.contains("MaterialComponent"))
        {
            MaterialComponent comp;
            comp.psoName = componentsJson["MaterialComponent"]["psoName"];
            auto color = componentsJson["MaterialComponent"]["color"];
            comp.color = Vector4(color[0], color[1], color[2], color[3]);
            registry->AddComponent<MaterialComponent>(newId, comp);
        }

        // HierarchyComponent の復元（IDの再マッピングが必須）
        if (componentsJson.contains("HierarchyComponent"))
        {
            HierarchyComponent comp;
            Entity oldParent = componentsJson["HierarchyComponent"]["parent"];

            // 親が kNullEntity でなく、かつ生成済みリストに存在する場合のみ新しいIDをセット
            if (oldParent != kNullEntity && idMap.find(oldParent) != idMap.end())
            {
                comp.parent = idMap[oldParent];
            }
            else
            {
                comp.parent = kNullEntity;
            }

            // 子エンティティのIDも新しいものに変換して復元
            for (Entity oldChild : componentsJson["HierarchyComponent"]["children"])
            {
                if (idMap.find(oldChild) != idMap.end())
                {
                    comp.children.push_back(idMap[oldChild]);
                }
            }
            registry->AddComponent<HierarchyComponent>(newId, comp);
        }

    }

    return true;
}