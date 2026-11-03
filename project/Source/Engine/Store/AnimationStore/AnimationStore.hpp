#pragma once
#include <memory>
#include <queue>
#include <unordered_map>
#include "AnimationData.hpp"

namespace Detail
{
	class AnimationStore
	{
	public:

		/// @brief コンストラクタ
		AnimationStore() = default;

		/// @brief デストラクタ
		~AnimationStore() = default;

		/// @brief 更新処理
		/// @param maxBufferCount 
		/// @param frameIndex 
		void Update(uint32_t maxBufferCount, uint32_t frameIndex);

		/// @brief 読み込み
		/// @param directory 
		/// @param fileName 
		/// @param device 
		/// @return 
		AnimationHandle Load(const std::string& filePath);

		/// @brief アニメーションの破棄
		/// @param handle 
		/// @param frameIndex 
		void Unload(AnimationHandle handle, uint32_t frameIndex);

		/// @brief アニメーションデータを取得する
		/// @param handle 
		/// @return 
		AnimationData* GetAnimationData(AnimationHandle handle);

		/// @brief アニメーションの数を取得する
		/// @return 
		size_t GetAnimationCount() const { return slots_.size(); }

		/// @brief インデックスでハンドルを取得する
		/// @param index 
		/// @return 
		AnimationHandle GetHandleByIndex(uint32_t index) const;


	private:

		// スロット内部情報
		struct Slot
		{
			// モデルデータ
			std::unique_ptr<AnimationData> data = nullptr;

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
		std::unordered_map<std::string, AnimationHandle> cache_;


	private:

		/// @brief 破棄を遅延処理する
		/// @param maxBufferCount 
		/// @param frameIndex 
		void ProcessDeferredDeletions(uint32_t maxBufferCount, uint32_t frameIndex);

		/// @brief 破棄待ちのデータ
		struct DeferredDeletionData
		{
			// 破棄するアニメーションデータ
			std::unique_ptr<AnimationData> data;

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