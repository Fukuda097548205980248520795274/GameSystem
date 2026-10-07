#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include "ECS/EntityECS/EntityECS.h"

namespace Detail
{
	class RegistryECS;

	class RenderSystem
	{
	public:

		/// @brief コンストラクタ
		RenderSystem() { Initialize(); }

		/// @brief エンティティを作成する
		/// @return 
		Entity CreateRender3DEntity();

		/// @brief すべての3D描画を実行する
		/// @param commandList 
		/// @param priority 
		void ExecuteAll(ID3D12GraphicsCommandList* commandList, int priority);


	private:

		/// @brief 初期化
		void Initialize();

		/// @brief ECSレジストリ
		RegistryECS* registry_ = nullptr;
	};
}