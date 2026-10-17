#pragma once
#include <memory>
#include "LightCullingPSO/LightCullingPSO.h"
#include "ECS/EntityECS/EntityECS.h"

#include "ECS/ComponentECS/DataForGPU/DataForGPU.h"
#include "Resource/DepthResource/DepthResource.h"
#include "Resource/ConstantBufferResource/ConstantBufferResource.h"
#include "Resource/StructuredBufferResource/StructuredBufferResource.h"
#include "Resource/RWStructuredBufferResource/RWStructuredBufferResource.h"

namespace Detail
{
	/// @brief ECSレジストリ
	class RegistryECS;

	/// @brief シェーダコンパイラ
	class ShaderCompiler;

	/// @brief レンダーシステム
	class RenderSystem;

	/// @brief ライトシステム
	class LightSystem
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param commandList 
		/// @param compiler 
		LightSystem(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ShaderCompiler* compiler) { Initialize(device, commandList, compiler); }

		/// @brief デストラクタ
		~LightSystem() = default;

		/// @brief リサイズ
		/// @param device 
		/// @param commandList 
		/// @param width 
		/// @param height 
		void Resize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, int32_t width, int32_t height);

		/// @brief 平行光源のエンティティを作成する
		/// @param direction 
		/// @param color 
		/// @param intensity 
		/// @return 
		Entity CreateDirectionalLightEntity(const Vector3& direction, const Vector4& color, float intensity);

		/// @brief ポイントライトのエンティティを作成する
		/// @param position 
		/// @param color 
		/// @param intensity 
		/// @param radius 
		/// @param decay 
		/// @return 
		Entity CreatePointLightEntity(const Vector3& position, const Vector4& color, float intensity, float radius, float decay);

		/// @brief カリング用カメラデータを取得する
		/// @return 
		ConstantBufferResource<LightCullingCameraDataForGPU>* GetCameraDataBuffer() const { return cameraDataBuffer_.get(); }

		/// @brief ポイントライトデータを取得する
		/// @return 
		StructuredBufferResource<PointLightDataForGPU>* GetPointLightDataBuffer()const { return pointLightDataBuffer_.get(); }

		/// @brief スポットライトデータを取得する
		/// @return 
		StructuredBufferResource<SpotLightDataForGPU>* GetSpotLightDataBuffer()const { return spotLightDataBuffer_.get(); }

		/// @brief ライトカリング用タイルデータを取得する
		/// @return 
		RWStructuredBufferResource<uint32_t>* GetTileLightIndexBuffer()const { return tileLightIndexBuffer_.get(); }

		/// @brief スポットライトのエンティティを作成する
		/// @param position 
		/// @param direction 
		/// @param color 
		/// @param intensity 
		/// @param radius 
		/// @param decay 
		/// @param innerConeAngle 
		/// @param outerConeAngle 
		/// @return 
		Entity CreateSpotLightEntity(const Vector3& position, const Vector3& direction, const Vector4& color, 
			float intensity, float radius, float decay, float cosAngle, float cosFalloffStart);

		/// @brief ライトカリング
		/// @param commandList 
		/// @param frameIndex 
		/// @param renderSystem 
		void LightCulling(ID3D12GraphicsCommandList* commandList, uint32_t frameIndex, RenderSystem* renderSystem);



	private:

		/// @brief 初期化
		/// @param device 
		/// @param commandList 
		/// @param compiler 
		void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ShaderCompiler* compiler);

		/// @brief ライトカリングPSO
		std::unique_ptr<LightCullingPSO> lightCullingPso_ = nullptr;

		/// @brief レジストリ
		RegistryECS* registry_;


	private:

		// 最大ライト数
		static constexpr int32_t kMaxNumLights = 1024;

		// タイル分割数
		static constexpr int32_t kTileSize = 16;

		/// @brief ライトカリング用の深度リソース
		std::unique_ptr<DepthResource> lightCullingDepthResource_;


		/// @brief カメラデータ用の定数バッファ
		std::unique_ptr<ConstantBufferResource<LightCullingCameraDataForGPU>> cameraDataBuffer_ = nullptr;

		/// @brief ライトデータ用の構造化バッファ
		std::unique_ptr<ConstantBufferResource<NumLightDataForGPU>> numLightDataBuffer_ = nullptr;

		/// @brief 平行光源用の構造化バッファ
		std::unique_ptr<StructuredBufferResource<DirectionalLightDataForGPU>> directionalLightDataBuffer_ = nullptr;

		/// @brief ポイントライトデータ用の構造化バッファ
		std::unique_ptr<StructuredBufferResource<PointLightDataForGPU>> pointLightDataBuffer_ = nullptr;

		/// @brief スポットライトデータ用の構造化バッファ
		std::unique_ptr<StructuredBufferResource<SpotLightDataForGPU>> spotLightDataBuffer_ = nullptr;

		/// @brief タイルライトインデックス用のRW構造化バッファ
		std::unique_ptr<RWStructuredBufferResource<uint32_t>> tileLightIndexBuffer_ = nullptr;
	};
}