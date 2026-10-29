#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include "Matrix/Matrix4x4/Matrix4x4.hpp"
#include "RenderResourceStore/RenderResourceStore.hpp"
#include "ECS/EntityECS/EntityECS.hpp"
#include "DepthOnlyPSO/DepthOnlyPSO.hpp"
#include "Store/ModelStore/ModelData.hpp"

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

		/// @brief モデルエンティティを作成する
		/// @param modelHandle 
		/// @return 
		Entity CreateModelEntity(ModelHandle modelHandle);

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
		/// @param device 
		/// @param shaderCompiler 
		void Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler);

		/// @brief グローバルなワールド行列を計算する
		/// @param entity 
		/// @return 
		Matrix4x4 CalculateGlobalWorldMatrix(Entity entity);

	private:

		/// @brief レンダーリソースストア
		std::unique_ptr<RenderResourceStore> renderResourceStore_;

		/// @brief 深度のみのPSO
		std::unique_ptr<DepthOnlyPSO> depthOnlyPso_ = nullptr;

		/// @brief PSOエディタ
		PSOEditor* psoEditor_ = nullptr;
	};
}