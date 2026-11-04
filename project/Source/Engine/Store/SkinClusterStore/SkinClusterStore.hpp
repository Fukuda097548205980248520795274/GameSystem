#pragma once
#include "SkinCluster/SkinCluster.hpp"
#include <queue>

namespace Detail
{
	class SkinClusterStore
	{
	public:

		/// @brief コンストラクタ
		SkinClusterStore() = default;

		/// @brief デストラクタ
		~SkinClusterStore() = default;

		/// @brief 更新処理
		/// @param maxBufferCount 
		/// @param frameIndex 
		void Update(uint32_t maxBufferCount, uint32_t frameIndex);

		/// @brief 読み込み
		/// @param directory 
		/// @param fileName 
		/// @param device 
		/// @return 
		SkinClusterHandle Load(DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshData& meshData, const Skeleton& skeleton);

		/// @brief スキンクラスターの破棄
		/// @param handle 
		/// @param frameIndex 
		void Unload(SkinClusterHandle handle, uint32_t frameIndex);

		/// @brief スキンクラスターを取得する
		/// @param handle 
		/// @return 
		SkinCluster* GetSkinCluster(SkinClusterHandle handle);

		/// @brief スキンクラスターの数を取得する
		/// @return 
		size_t GetSkinClusterCount() const { return slots_.size(); }

		/// @brief インデックスでハンドルを取得する
		/// @param index 
		/// @return 
		SkinClusterHandle GetHandleByIndex(uint32_t index) const;


	private:

		// スロット内部情報
		struct Slot
		{
			// モデルデータ
			std::unique_ptr<SkinCluster> data = nullptr;

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
		std::vector<SkinClusterHandle> cache_;


	private:

		/// @brief 破棄を遅延処理する
		/// @param maxBufferCount 
		/// @param frameIndex 
		void ProcessDeferredDeletions(uint32_t maxBufferCount, uint32_t frameIndex);

		/// @brief 破棄待ちのデータ
		struct DeferredDeletionData
		{
			// 破棄するスケルトンデータ
			std::unique_ptr<SkinCluster> data;

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