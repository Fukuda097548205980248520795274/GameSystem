#pragma once
#include "SkinningInstance/SkinningInstance.hpp"
#include <queue>

namespace Detail
{
	class SkinningInstanceStore
	{
	public:

		/// @brief コンストラクタ
		SkinningInstanceStore() = default;

		/// @brief デストラクタ
		~SkinningInstanceStore() = default;

		/// @brief 更新処理
		/// @param maxBufferCount 
		/// @param frameIndex 
		void Update(uint32_t maxBufferCount, uint32_t frameIndex);

		/// @brief 読み込み
		/// @param directory 
		/// @param fileName 
		/// @param device 
		/// @return 
		SkinningInstanceHandle Load(DXHeap* heap, ID3D12Device* device, const Skeleton& skeleton, const MeshData& meshData, const MeshBoneData& meshBoneData);

		/// @brief スキニングインスタンスの破棄
		/// @param handle 
		/// @param frameIndex 
		void Unload(SkinningInstanceHandle handle, uint32_t frameIndex);

		/// @brief スキニングインスタンスデータを取得する
		/// @param handle 
		/// @return 
		SkinningInstance* GetSkinningInstance(SkinningInstanceHandle handle);

		/// @brief スキニングインスタンスの数を取得する
		/// @return 
		size_t GetSkinningInstanceCount() const { return slots_.size(); }

		/// @brief インデックスでハンドルを取得する
		/// @param index 
		/// @return 
		SkinningInstanceHandle GetHandleByIndex(uint32_t index) const;


	private:

		// スロット内部情報
		struct Slot
		{
			// モデルデータ
			std::unique_ptr<SkinningInstance> data = nullptr;

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
		std::vector<SkinningInstanceHandle> cache_;


	private:

		/// @brief 破棄を遅延処理する
		/// @param maxBufferCount 
		/// @param frameIndex 
		void ProcessDeferredDeletions(uint32_t maxBufferCount, uint32_t frameIndex);

		/// @brief 破棄待ちのデータ
		struct DeferredDeletionData
		{
			// 破棄するスケルトンデータ
			std::unique_ptr<SkinningInstance> data;

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