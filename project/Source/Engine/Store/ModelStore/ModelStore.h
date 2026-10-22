#pragma once
#include "ModelData.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <queue>

namespace Detail
{
	/// @brief テクスチャ
	class TextureStore;

	/// @brief ヒープ
	class DXHeap;

	/// @brief モデルストア
	class ModelStore
	{
	public:

		/// @brief コンストラクタ
		ModelStore() = default;

		/// @brief デストラクタ
		~ModelStore() = default;

		/// @brief 読み込み
		/// @param directory 
		/// @param fileName 
		/// @param device 
		/// @return 
		ModelHandle Load(const std::string& filePath, TextureStore* textureStore, DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

		/// @brief モデルの破棄
		/// @param handle 
		void Unload(ModelHandle handle);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param modelHandle 
		/// @param meshIndex 
		void Register(ID3D12GraphicsCommandList* commandList, ModelHandle handle, int32_t meshIndex);

		/// @brief モデルデータを取得する
		/// @param handle 
		/// @return 
		ModelData* GetModelData(ModelHandle handle);


	private:

		// スロット内部情報
		struct Slot
		{
			// モデルデータ
			std::unique_ptr<ModelData> data = nullptr;

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
		std::unordered_map<std::string, ModelHandle> cache_;

	};
}