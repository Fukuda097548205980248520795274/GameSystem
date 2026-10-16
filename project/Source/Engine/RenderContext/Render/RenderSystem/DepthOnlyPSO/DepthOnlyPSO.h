#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <dxcapi.h>

namespace Detail
{
	/// @brief シェーダコンパイラ
	class ShaderCompiler;

	/// @brief 深度のみPSO
	class DepthOnlyPSO
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param compiler 
		DepthOnlyPSO(ID3D12Device* device, ShaderCompiler* compiler) { Initialize(device, compiler); }

		/// @brief 仮想デストラクタ
		~DepthOnlyPSO() = default;

		/// @brief コマンドリストに登録する
		/// @param commandList 
		void Register(ID3D12GraphicsCommandList* commandList);


		template<class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;


	protected:


		/// @brief 初期化
		/// @param device 
		/// @param compiler 
		/// @param vertexShaderBlob 
		/// @param log 
		void Initialize(ID3D12Device* device, ShaderCompiler* compiler);


		// 頂点シェーダのバイナリデータ
		ComPtr<IDxcBlob> vertexShaderBlob_ = nullptr;

		// シグネチャのバイナリデータ
		ComPtr<ID3DBlob> signatureBlob_ = nullptr;

		// エラーのバイナリデータ
		ComPtr<ID3DBlob> errorBlob_ = nullptr;

		// ルートシグネチャ
		ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

		// PSO
		ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;
	};
}