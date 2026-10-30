#pragma once
#include <string>
#include <json.hpp>

namespace Detail
{
	// ECSレジストリ
	class RegistryECS;

	/// @brief json形式にシリアライズする
	/// @param registry 
	/// @param sceneName 
	/// @return 
	bool ToJson(RegistryECS* registry, const std::string& sceneName = "Scene");

	/// @brief jsonファイルからロードする
	/// @param registry 
	/// @param sceneName 
	/// @return 
	bool LoadJson(RegistryECS* registry, const std::string& sceneName = "Scene");

	/// @brief バイナリファイルからロードする
	/// @param registry 
	/// @param sceneName 
	/// @return 
	bool LoadBin(RegistryECS* registry, const std::string& sceneName = "Scene");

	/// @brief jsonデータからのエンティティ／コンポーネント再構築処理
	/// @param registry 
	/// @param rootJson 
	/// @return 
	bool Deserialize(RegistryECS* registry, const nlohmann::json& rootJson);
}