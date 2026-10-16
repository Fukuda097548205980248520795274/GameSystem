#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include "RenderResourceStore/RenderResourceStore.h"
#include "ECS/EntityECS/EntityECS.h"
#include "DepthOnlyPSO/DepthOnlyPSO.h"

namespace Detail
{
	/// @brief ECSレジストリ
	class RegistryECS;

	/// @brief PSOエディタ
	class PSOEditor;

	/// @brief シェーダコンパイラ
	class ShaderCompiler;

	/// @brief レンダーシステム
	class RenderSystem
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param shaderCompiler 
		RenderSystem(ID3D12Device* device, ShaderCompiler* shaderCompiler) { Initialize(device, shaderCompiler); }

		/// @brief エンティティを作成する
		/// @return 
		Entity CreateRender3DEntity();

		/// @brief 描画前の深度パスを実行する
		/// @param commandList 
		/// @param priority 
		void ExecuteDepthPrePass(ID3D12GraphicsCommandList* commandList, int32_t frameIndex = 1);

		/// @brief すべての3D描画を実行する
		/// @param commandList 
		/// @param priority 
		void ExecuteAll(ID3D12GraphicsCommandList* commandList, int priority);


	private:

		/// @brief 初期化
		void Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler);

		/// @brief レンダーリソースストア
		std::unique_ptr<RenderResourceStore> renderResourceStore_;

		/// @brief 深度のみのPSO
		std::unique_ptr<DepthOnlyPSO> depthOnlyPso_ = nullptr;

		/// @brief ECSレジストリ
		RegistryECS* registry_ = nullptr;

		/// @brief PSOエディタ
		PSOEditor* psoEditor_ = nullptr;
	};
}