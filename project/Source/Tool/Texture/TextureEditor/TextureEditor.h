#pragma once
#include <string>
#include <cstdint>
#include <mutex>
#include <vector>
#include "Store/TextureStore/TextureData.h"

namespace Detail
{
	/// @brief レンダーコンテキスト
	class RenderContext;

	/// @brief テクスチャストア
	class TextureStore;

	/// @brief テクスチャエディタ
	class TextureEditor
	{
	public:

		/// @brief コンストラクタ
		TextureEditor() = default;

		/// @brief デストラクタ
		~TextureEditor() = default;

		/// @brief UIを描画する
		/// @param renderContext 
		/// @param textureStore 
		void DrawUI(RenderContext* renderContext, TextureStore* textureStore);

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

		// 選択中のテクスチャハンドル
		TextureHandle selectedHandle_ = TextureHandle::Invalid();


	private:

		// デフォルトのテクスチャディレクトリ
		const std::string kDir = "./Cooked/Texture/";
	};
}