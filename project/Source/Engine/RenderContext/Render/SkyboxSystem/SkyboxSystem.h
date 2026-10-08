#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include "ECS/EntityECS/EntityECS.h"

namespace Detail
{
	class RegistryECS;

	class SkyboxSystem
	{
	public:

		/// @brief コンストラクタ
		SkyboxSystem() { Initialize(); }

		/// @brief エンティティを作成する
		/// @return 
		Entity CreateSkyboxEntity();

		/// @brief すべての3D描画を実行する
		/// @param commandList 
		/// @param priority 
		void Execute(ID3D12GraphicsCommandList* commandList);


	private:

		/// @brief 初期化
		void Initialize();

		/// @brief ECSレジストリ
		RegistryECS* registry_ = nullptr;
	};
}