#pragma once
#include <memory>
#include "Matrix/Matrix4x4/Matrix4x4.hpp"
#include "SkyboxPSO/SkyboxPSO.hpp"
#include "ECS/EntityECS/EntityECS.hpp"
#include "Resource/VertexBufferResource/VertexBufferResource.hpp"
#include "Resource/IndexBufferResource/IndexBufferResource.hpp"
#include "Resource/ConstantBufferResource/ConstantBufferResource.hpp"

namespace Detail
{
	/// @brief ECSレジストリ
	class RegistryECS;

	/// @brief シェーダコンパイラ
	class ShaderCompiler;

	/// @brief スカイボックスシステム
	class SkyboxSystem
	{
	public:

		/// @brief コンストラクタ
		SkyboxSystem(ID3D12Device* device, ShaderCompiler* shaderCompiler) { Initialize(device, shaderCompiler); }

		/// @brief デストラクタ
		~SkyboxSystem() = default;

		/// @brief エンティティを作成する
		/// @return 
		Entity CreateSkyboxEntity();

		/// @brief すべての3D描画を実行する
		/// @param commandList 
		/// @param priority 
		void Execute(ID3D12GraphicsCommandList* commandList);


	private:

		/// @brief 初期化
		void Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler);

		/// @brief ECSレジストリ
		RegistryECS* registry_ = nullptr;

		/// @brief SkyboxPSO
		std::unique_ptr<SkyboxPSO> pso_;


	private:

		// 頂点リソース
		std::unique_ptr<VertexBufferResource<Vector4>> vertexResource_ = nullptr;

		// インデックスリソース
		std::unique_ptr<IndexBufferResource> indexResource_ = nullptr;

		/// @brief 変換行列バッファ
		std::unique_ptr<ConstantBufferResource<Matrix4x4>> transformBuffer_ = nullptr;

		/// @brief カラーバッファ
		std::unique_ptr<ConstantBufferResource<Vector4>> colorBuffer_ = nullptr;
	};
}