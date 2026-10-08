#pragma once
#include <string>
#include <cstdint>
#include <mutex>
#include <vector>

namespace Detail
{
	class RenderContext;
	class TextureStore;

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
		uint32_t selectedHandle_ = UINT32_MAX;


	private:

		// デフォルトのテクスチャディレクトリ
		const std::string kDir = "./Assets/Texture/";
	};
}