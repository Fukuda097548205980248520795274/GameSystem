#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>

#include "SkeletonData.hpp"

namespace Detail
{
	/// @brief スケルトンストア
	class SkeletonStore
	{
	public:

		/// @brief コンストラクタ
		SkeletonStore() = default;

		/// @brief デストラクタ
		~SkeletonStore() = default;

		/// @brief 更新処理
		/// @param maxBufferCount 
		/// @param frameIndex 
		void Update(uint32_t maxBufferCount, uint32_t frameIndex);

		/// @brief 読み込み
		/// @param directory 
		/// @param fileName 
		/// @param device 
		/// @return 
		SkeletonHandle Load(const std::string& filePath);

		/// @brief スケルトンの破棄
		/// @param handle 
		/// @param frameIndex 
		void Unload(SkeletonHandle handle, uint32_t frameIndex);

		/// @brief スケルトンデータを取得する
		/// @param handle 
		/// @return 
		SkeletonData* GetSkeletonData(SkeletonHandle handle);

		/// @brief スケルトンの数を取得する
		/// @return 
		size_t GetSkeletonCount() const { return slots_.size(); }

		/// @brief インデックスでハンドルを取得する
		/// @param index 
		/// @return 
		SkeletonHandle GetHandleByIndex(uint32_t index) const;


	private:

		// スロット内部情報
		struct Slot
		{
			// モデルデータ
			std::unique_ptr<SkeletonData> data = nullptr;

			// スロットの世代番号
			uint32_t generation = 1;

			// スロットが使用中かどうか
			bool inUse = false;
		};

		// データテーブル
		std::vector<Slot> slots_;

		// 空きスロットのインデックスキュー
		std::queue<uint32_t> freeSlots_;

		// キャッシュ
		std::unordered_map<std::string, SkeletonHandle> cache_;


	private:

		/// @brief 破棄を遅延処理する
		/// @param maxBufferCount 
		/// @param frameIndex 
		void ProcessDeferredDeletions(uint32_t maxBufferCount, uint32_t frameIndex);

		/// @brief 破棄待ちのデータ
		struct DeferredDeletionData
		{
			// 破棄するスケルトンデータ
			std::unique_ptr<SkeletonData> data;

			// 破棄を要求したフレームインデックス
			uint32_t requestedFrameIndex;
		};

		// 破棄待ちキュー
		std::queue<DeferredDeletionData> deferredDeletionQueue_;


	private:

		// 出力用ディレクトリ
		const std::string kCookedDir = "./Cooked/Model/";
	};
}