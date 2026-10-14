#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <dxcapi.h>

namespace Detail
{
	class ShaderCompiler;

	class LightCullingPSO
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param compiler 
		LightCullingPSO(ID3D12Device* device, ShaderCompiler* compiler) { Initialize(device, compiler); }

		/// @brief デストラクタ
		~LightCullingPSO() = default;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		void Register(ID3D12GraphicsCommandList* commandList);

		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;


	protected:

		/// @brief 初期化
		/// @param device 
		/// @param compiler 
		void Initialize(ID3D12Device* device, ShaderCompiler* compiler);


		// コンピュートシェーダのバイナリ
		ComPtr<IDxcBlob> computeShaderBlob_ = nullptr;

		// シグネチャのバイナリデータ
		ComPtr<ID3DBlob> signatureBlob_ = nullptr;

		// エラーのバイナリデータ
		ComPtr<ID3DBlob> errorBlob_ = nullptr;

		// ルートシグネチャ
		ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;


		// パイプライン
		ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;
	};
}