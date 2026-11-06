#include "SkinningPSO.hpp"
#include "Engine.hpp"

/// @brief 初期化
/// @param device 
/// @param compiler 
void Detail::SkinningPSO::Initialize(ID3D12Device* device, ShaderCompiler* compiler)
{
	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();

	// シェーダのバイナリデータを取得する
	computeShaderBlob_ = compiler->Compile(L"./Assets/Internal/Shader/Skinning/Skinning.CS.hlsl", L"cs_6_0");
	assert(computeShaderBlob_ != nullptr);


	/*-----------------------
		ディスクリプタレンジ
	-----------------------*/

	D3D12_DESCRIPTOR_RANGE descriptorWell[1];
	descriptorWell[0].BaseShaderRegister = 0;
	descriptorWell[0].RegisterSpace = 0;
	descriptorWell[0].NumDescriptors = 1;
	descriptorWell[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorWell[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE descriptorVertex[1];
	descriptorVertex[0].BaseShaderRegister = 1;
	descriptorVertex[0].RegisterSpace = 0;
	descriptorVertex[0].NumDescriptors = 1;
	descriptorVertex[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorVertex[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE descriptorVertexInfluence[1];
	descriptorVertexInfluence[0].BaseShaderRegister = 2;
	descriptorVertexInfluence[0].RegisterSpace = 0;
	descriptorVertexInfluence[0].NumDescriptors = 1;
	descriptorVertexInfluence[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorVertexInfluence[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE descriptorOutputVertices[1];
	descriptorOutputVertices[0].BaseShaderRegister = 0;
	descriptorOutputVertices[0].RegisterSpace = 0;
	descriptorOutputVertices[0].NumDescriptors = 1;
	descriptorOutputVertices[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	descriptorOutputVertices[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



	/*---------------------
		ルートパラメータ
	---------------------*/

	D3D12_ROOT_PARAMETER rootParameter[5];

	// SRV DescriptorTable t0
	rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[0].DescriptorTable.pDescriptorRanges = descriptorWell;
	rootParameter[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorWell);

	// SRV DescriptorTable t1
	rootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[1].DescriptorTable.pDescriptorRanges = descriptorVertex;
	rootParameter[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorVertex);

	// SRV DescriptorTable t2
	rootParameter[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[2].DescriptorTable.pDescriptorRanges = descriptorVertexInfluence;
	rootParameter[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorVertexInfluence);

	// UAV DescriptorTable u0
	rootParameter[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[3].DescriptorTable.pDescriptorRanges = descriptorOutputVertices;
	rootParameter[3].DescriptorTable.NumDescriptorRanges = _countof(descriptorOutputVertices);

	// CBV b0
	rootParameter[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter[4].Descriptor.RegisterSpace = 0;
	rootParameter[4].Descriptor.ShaderRegister = 0;


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
	if (!SUCCEEDED(hr))
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
	if (!SUCCEEDED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, "Failed to create compute pipeline state.");
		assert(false);
	}
}

/// @brief コマンドリストに登録する
/// @param commandList 
void Detail::SkinningPSO::Register(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->SetComputeRootSignature(rootSignature_.Get());
}