#pragma once
#include <memory>
#include "LightCullingPSO/LightCullingPSO.h"

#include "ECS/ComponentECS/DataForGPU/DataForGPU.h"
#include "Resource/ConstantBufferResource/ConstantBufferResource.h"
#include "Resource/StructuredBufferResource/StructuredBufferResource.h"
#include "Resource/RWStructuredBufferResource/RWStructuredBufferResource.h"

namespace Detail
{
	class RegistryECS;
	class ShaderCompiler;

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


		/// @brief カメラデータ用の定数バッファ
		std::unique_ptr<ConstantBufferResource<LightCullingCameraDataForGPU>> cameraDataBuffer_ = nullptr;

		/// @brief ライトデータ用の構造化バッファ
		std::unique_ptr<ConstantBufferResource<NumLightDataForGPU>> numLightDataBuffer_ = nullptr;

		/// @brief ポイントライトデータ用の構造化バッファ
		std::unique_ptr<StructuredBufferResource<PointLightDataForGPU>> pointLightDataBuffer_ = nullptr;

		/// @brief スポットライトデータ用の構造化バッファ
		std::unique_ptr<StructuredBufferResource<SpotLightDataForGPU>> spotLightDataBuffer_ = nullptr;

		/// @brief タイルライトインデックス用のRW構造化バッファ
		std::unique_ptr<RWStructuredBufferResource<uint32_t>> tileLightIndexBuffer_ = nullptr;
	};
}