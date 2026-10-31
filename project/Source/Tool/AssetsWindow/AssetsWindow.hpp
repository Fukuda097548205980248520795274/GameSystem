#pragma once
#include <string>
#include <cstdint>
#include <mutex>
#include <vector>
#include <filesystem>
#include "Store/TextureStore/TextureData.hpp"
#include "Store/ModelStore/ModelData.hpp"
#include "Script/ScriptManager/ScriptManager.hpp"

namespace Detail
{
	// レンダーコンテキスト
	class RenderContext;

	// テクスチャストア
	class TextureStore;

	// モデルストア
	class ModelStore;

	// スクリプトマネージャ
	class ScriptManager;

	// 将来的なアセット追加を見据えたカテゴリ
	enum class AssetCategory
	{
		Texture,
		Model,
		Audio,
		Script
	};

	/// @brief アセットウィンドウ
	class AssetsWindow
	{
	public:

		// コンストラクタ
		AssetsWindow() = default;

		// デストラクタ
		~AssetsWindow() = default;

		/// @brief UI描画
		/// @param renderContext 
		/// @param textureStore 
		/// @param modelStore 
		/// @param scriptManager 
		void DrawUI(RenderContext* renderContext, TextureStore* textureStore, ModelStore* modelStore, ScriptManager* scriptManager);

		/// @brief ドロップされたファイルの処理
		/// @param filePath 
		void OnFileDropped(const std::string& filePath);

	private:

		/// @brief ドロップされたファイルの処理
		/// @param renderContext 
		/// @param scriptManager 
		void ProcessDroppedFiles(RenderContext* renderContext, ScriptManager* scriptManager);

		/// @brief テクスチャタブの描画
		/// @param renderContext 
		/// @param textureStore 
		void DrawTextureTab(RenderContext* renderContext, TextureStore* textureStore);

		/// @brief モデルタブの描画
		/// @param renderContext 
		/// @param modelStore 
		void DrawModelTab(RenderContext* renderContext, ModelStore* modelStore);

		/// @brief スクリプトタブの描画
		/// @param scriptManager 
		void DrawScriptTab(ScriptManager* scriptManager);

	private:

		// ドロップされたファイルのキュー
		std::vector<std::string> droppedFilesQueue_;

		// ドロップ処理の排他制御
		std::mutex dropMutex_;

		// 現在表示中のカテゴリ
		AssetCategory currentCategory_ = AssetCategory::Texture;

		// 選択中のハンドル
		TextureHandle selectedTextureHandle_ = TextureHandle::Invalid();
		ModelHandle selectedModelHandle_ = ModelHandle::Invalid();
		ScriptHandle selectedScriptHandle_ = ScriptHandle::Invalid();

		// 各アセットの保存ディレクトリ
		const std::string kTextureDir = "./Cooked/Texture/";
		const std::string kModelDir = "./Assets/Model/";
		const std::string kScriptDir = "./Assets/Script/";
	};
}