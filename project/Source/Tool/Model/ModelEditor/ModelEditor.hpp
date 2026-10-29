#pragma once
#include <string>
#include <cstdint>
#include <mutex>
#include <vector>
#include "Store/ModelStore/ModelData.hpp"

namespace Detail
{
	/// @brief レンダーコンテキスト
	class RenderContext;

	/// @brief モデルストア
	class ModelStore;

	/// @brief モデルエディタ
	class ModelEditor
	{
	public:

		/// @brief コンストラクタ
		ModelEditor() = default;

		/// @brief デストラクタ
		~ModelEditor() = default;

		/// @brief UIを描画する
		/// @param renderContext 
		/// @param modelStore 
		void DrawUI(RenderContext* renderContext, ModelStore* modelStore);

		/// @brief ドロップされたファイルを処理する
		/// @param filePath 
		void OnFileDropped(const std::string& filePath);


	private:

		/// @brief ドロップされたファイルのキュー
		std::vector<std::string> droppedFilesQueue_;

		/// @brief ドロップされたファイルを処理する
		std::mutex dropMutex_;

		// 入力用のバッファ
		char inputFilePath_[256] = "";

		// 選択中のモデルハンドル
		ModelHandle selectedHandle_ = ModelHandle::Invalid();


	private:

		// デフォルトのモデルディレクトリ
		const std::string kDir = "./Assets/Model/";
	};
}