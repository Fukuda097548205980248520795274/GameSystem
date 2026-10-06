#pragma once
#include <wrl.h>
#include "PSO/PSODescription/PSODescription.h"

namespace Detail
{
	class ShaderCompiler;

	class DynamicPSOBuilder
	{
	public:

		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		/// @brief 設定データから動的にPSOとRootSignatureを生成する
		/// @param device 
		/// @param compiler 
		/// @param desc 
		/// @param defaultRootSignature 
		/// @param outPipelineState 
		/// @return 
		bool Build(ID3D12Device* device, ShaderCompiler* compiler, const PSODescription& desc, ID3D12RootSignature** outRootSignature, ID3D12PipelineState** outPipelineState);

		/// @brief ブレンドモードのプリセットを取得する
		/// @param blendMode 
		/// @return 
		static CustomBlendDesc GetPresetBlendDesc(EditorBlendMode blendMode);


	private:

		/// @brief ブレンドモードを作成する
		/// @param blendMode 
		/// @return 
		D3D12_BLEND_DESC CreateBlendMode(EditorBlendMode blendMode);


		/// @brief ブレンドモード作成 : 合成なし
		/// @return 
		D3D12_BLEND_DESC CreateBlendNone();

		/// @brief ブレンドモード作成 : ノーマル合成
		/// @return 
		D3D12_BLEND_DESC CreateBlendNormal();

		/// @brief ブレンドモード作成 : 加算合成
		/// @return 
		D3D12_BLEND_DESC CreateBlendAdd();

		/// @brief ブレンドモード作成 : 減算合成
		/// @return 
		D3D12_BLEND_DESC CreateBlendSubtract();

		/// @brief ブレンドモード作成 : 乗算合成
		/// @return 
		D3D12_BLEND_DESC CreateBlendMultiply();

		/// @brief ブレンドモード作成 : スクリーン合成
		/// @return 
		D3D12_BLEND_DESC CreateBlendScreen();

		/// @brief ブレンドモード作成 : カスタム合成
		/// @param desc 
		/// @return 
		D3D12_BLEND_DESC CreateBlendMode(const PSODescription& desc);
	};
}