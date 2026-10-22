#pragma once
#include "ModelData.h"
#include <d3d12.h>
#include <dxgi1_6.h>

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
		uint32_t Load(const std::string& filePath, TextureStore* textureStore, DXHeap* heap, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param modelHandle 
		/// @param meshIndex 
		void Register(ID3D12GraphicsCommandList* commandList, uint32_t modelHandle, int32_t meshIndex);

		/// @brief モデルデータを取得する
		/// @param handle 
		/// @return 
		ModelData* GetModelData(uint32_t handle) { return dataTable_[handle].get(); }


	private:

		// データテーブル
		std::vector<std::unique_ptr<ModelData>> dataTable_;

	};
}