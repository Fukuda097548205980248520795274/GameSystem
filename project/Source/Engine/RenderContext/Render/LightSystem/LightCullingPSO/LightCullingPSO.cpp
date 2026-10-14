#include "LightCullingPSO.h"
#include "Engine.h"

/// @brief 初期化
/// @param device 
/// @param compiler 
void Detail::LightCullingPSO::Initialize(ID3D12Device* device, ShaderCompiler* compiler)
{
	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();

	// シェーダのバイナリデータを取得する
	computeShaderBlob_ = compiler->Compile(L"./Assets/Internal/Shader/LightCulling/LightCulling.CS.hlsl", L"cs_6_0");
	assert(computeShaderBlob_ != nullptr);


	/*-----------------------
		ディスクリプタレンジ
	-----------------------*/

	D3D12_DESCRIPTOR_RANGE pointLightData[1];
	pointLightData[0].BaseShaderRegister = 0;
	pointLightData[0].RegisterSpace = 0;
	pointLightData[0].NumDescriptors = 1;
	pointLightData[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pointLightData[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE spotLightData[1];
	spotLightData[0].BaseShaderRegister = 1;
	spotLightData[0].RegisterSpace = 0;
	spotLightData[0].NumDescriptors = 1;
	spotLightData[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	spotLightData[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE depthTexture[1];
	depthTexture[0].BaseShaderRegister = 2;
	depthTexture[0].RegisterSpace = 0;
	depthTexture[0].NumDescriptors = 1;
	depthTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	depthTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE titleLightIndices[1];
	titleLightIndices[0].BaseShaderRegister = 0;
	titleLightIndices[0].RegisterSpace = 0;
	titleLightIndices[0].NumDescriptors = 1;
	titleLightIndices[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	titleLightIndices[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



	/*---------------------
		ルートパラメータ
	---------------------*/

	D3D12_ROOT_PARAMETER rootParameter[6];

	// CBV b0
	rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[0].Descriptor.ShaderRegister = 0;
	rootParameter[0].Descriptor.RegisterSpace = 0;
	
	// CBV b1
	rootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[1].Descriptor.ShaderRegister = 1;
	rootParameter[1].Descriptor.RegisterSpace = 0;

	// SRV DescriptorTable t0
	rootParameter[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[2].DescriptorTable.pDescriptorRanges = pointLightData;
	rootParameter[2].DescriptorTable.NumDescriptorRanges = _countof(pointLightData);

	// SRV DescriptorTable t1
	rootParameter[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[3].DescriptorTable.pDescriptorRanges = spotLightData;
	rootParameter[3].DescriptorTable.NumDescriptorRanges = _countof(spotLightData);

	// SRV DescriptorTable t2
	rootParameter[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[4].DescriptorTable.pDescriptorRanges = depthTexture;
	rootParameter[4].DescriptorTable.NumDescriptorRanges = _countof(depthTexture);

	// UAV DescriptorTable u0
	rootParameter[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[5].DescriptorTable.pDescriptorRanges = titleLightIndices;
	rootParameter[5].DescriptorTable.NumDescriptorRanges = _countof(titleLightIndices);


	/*-------------------------
		ルートシグネチャの生成
	-------------------------*/

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// ルートパラメータを設定する
	descriptionRootSignature.pParameters = rootParameter;
	descriptionRootSignature.NumParameters = _countof(rootParameter);

	// シリアライズしてバイナリにする
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob_, &errorBlob_);

	// エラーのとき、情報を出力し停止させる
	if (FAILED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, (char*)errorBlob_->GetBufferPointer());
		assert(false);
	}

	// バイナリを元に生成
	hr = device->CreateRootSignature(0,
		signatureBlob_->GetBufferPointer(), signatureBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	if(!SUCCEEDED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, "Failed to create root signature.");
		assert(false);
	}


	/*------------------
		PSOを生成する
	------------------*/

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc{};
	computePipelineStateDesc.CS = { computeShaderBlob_->GetBufferPointer(), computeShaderBlob_->GetBufferSize() };
	computePipelineStateDesc.pRootSignature = rootSignature_.Get();

	hr = device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&pipelineState_));
	if(!SUCCEEDED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, "Failed to create compute pipeline state.");
		assert(false);
	}
}