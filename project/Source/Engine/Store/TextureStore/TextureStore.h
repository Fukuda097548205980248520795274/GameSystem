#pragma once
#include "ECS/ComponentECS/ComponentECS.h"
#include "TextureData.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <queue>

namespace Detail
{
	/// @brief テクスチャストア
	class TextureStore
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param heap 
		/// @param commandList 
		TextureStore(ID3D12Device* device, DXHeap* heap, ID3D12GraphicsCommandList* commandList){ Initialize(device, heap, commandList); }

		/// @brief 更新処理
		/// @param maxBufferCount 
		/// @param frameIndex 
		void Update(uint32_t maxBufferCount, uint32_t frameIndex);

		/// @brief テクスチャを読み込む
		/// @param filePath 
		/// @param heap 
		/// @param device 
		/// @param commandList 
		/// @return 
		TextureHandle Load(const std::string& filePath, DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

		/// @brief ファイルパスを取得する
		/// @param handle 
		/// @return 
		std::string GetFilePath(TextureHandle handle) const;

		/// @brief SRV用GPUハンドルを取得する
		/// @param handle 
		/// @return 
		SRVDescriptorHandle GetSrvHandle(TextureHandle handle) const;

		/// @brief テクスチャの幅を取得する
		/// @param handle 
		/// @return 
		size_t GetTextureWidth(TextureHandle handle) const;

		/// @brief テクスチャの高さを取得する
		/// @param handle 
		/// @return 
		size_t GetTextureHeight(TextureHandle handle) const;

		/// @brief ハンドルを取得する
		/// @param filePath 
		/// @return 
		TextureHandle GetHandle(const std::string& filePath) const;

		/// @brief テクスチャの種類を取得する
		/// @param handle 
		/// @return 
		TextureType GetType(TextureHandle handle) const;

		/// @brief ハンドルのインデックスを取得する
		/// @param index 
		/// @return 
		TextureHandle GetHandleByIndex(uint32_t index) const;

		/// @brief テクスチャの数を取得する
		/// @return 
		size_t GetTextureCount() const { return slots_.size(); }

		/// @brief テクスチャを削除する
		/// @param handle 
		void Remove(TextureHandle handle, DXHeap* heap, uint32_t frameIndex);

		/// @brief 中間リソースを解放する
		void ReleaseIntermediateResources();

		/// @brief デフォルトテクスチャハンドルを取得する
		/// @return 
		TextureHandle GetDefaultTextureHandle() const { return defaultTextureHandle_; }

		/// @brief デフォルトキューブマップハンドルを取得する
		/// @return 
		TextureHandle GetDefaultCubemapHandle() const { return defaultCubemapHandle_; }


	private:

		/// @brief 初期化
		void Initialize(ID3D12Device* device, DXHeap* heap, ID3D12GraphicsCommandList* commandList);

		/// @brief パスを正規化する
		/// @param path 
		/// @return 
		std::string NormalizePath(const std::string& path) const;

		/// @brief テクスチャのハッシュ値を計算する
		/// @param image 
		/// @return 
		uint64_t ComputeImageHash(const DirectX::ScratchImage& image) const;


	private:

		// スロット内部情報
		struct Slot
		{
			// テクスチャデータ
			std::unique_ptr<TextureData> data = nullptr;

			// スロットの世代番号
			uint32_t generation = 1;

			// スロットが使用中かどうか
			bool inUse = false;
		};

		/// @brief テクスチャデータを取得する
		/// @param handle 
		/// @return 
		TextureData* GetTextureData(TextureHandle handle) const;


	private:

		// データテーブル
		std::vector<Slot> slots_;

		// 空きスロットのインデックスキュー
		std::queue<uint32_t> freeSlots_;

		// ファイルパス / ハンドルのマップ
		std::unordered_map<std::string, TextureHandle> pathToHandleMap_;

		// ハッシュ値 / ハンドルのマップ
		std::unordered_map<uint64_t, TextureHandle> hashToHandleMap_;

		// 中間リソースのアップロード待ちリスト
		std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint32_t>> pendingUploadResources_;


	private:

		/// @brief ファイルのハッシュ値を計算する
		/// @param filePath
		/// @return
		uint64_t ComputeFileHash(const std::string& filePath) const;

		/// @brief DDSに変換する
		/// @param sourcePath 
		/// @param destDir 
		/// @param outFileName
		/// @return 
		bool ConvertToDDS(const std::string& sourcePath, const std::string& destDir, const std::string& outFileName) const;

		// DirectXTexのtexconv.exeの相対パス
		const std::string kTexConvDir = "./Externals/DirectXTex/texconv.exe"; 

		// 出力用のテクスチャディレクトリ
		const std::string kCookedTextureDir = "./Cooked/Texture/";

		// texconv.exeの引数
		const std::string kTexConvArgs = "-y -f BC7_UNORM_SRGB -m 1 -srgb";


	private:

		/// @brief 破棄を遅延処理する
		/// @param maxBufferCount 
		/// @param frameIndex 
		void ProcessDeferredDeletions(uint32_t maxBufferCount, uint32_t frameIndex);

		/// @brief 破棄待ちのデータ
		struct DeferredDeletionData
		{
			std::unique_ptr<TextureData> data;
			DXHeap* heap;
			uint32_t requestedFrameIndex;
		};

		// 破棄待ちキュー
		std::queue<DeferredDeletionData> deferredDeletionQueue_;


	private:

		// 初期テクスチャハンドル
		TextureHandle defaultTextureHandle_ = TextureHandle::Invalid();

		// 初期キューブマップハンドル
		TextureHandle defaultCubemapHandle_ = TextureHandle::Invalid();

	};
}