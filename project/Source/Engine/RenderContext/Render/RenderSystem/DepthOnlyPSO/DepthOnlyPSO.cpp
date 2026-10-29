#include "DepthOnlyPSO.hpp"
#include "Engine.hpp"

/// @brief 初期化
/// @param device 
/// @param compiler 
void Detail::DepthOnlyPSO::Initialize(ID3D12Device* device, ShaderCompiler* compiler)
{
	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	// 頂点シェーダを読み込む
	vertexShaderBlob_ = compiler->Compile(L"./Assets/Internal/Shader/DepthOnly/DepthOnly.VS.hlsl", L"vs_6_0");
	if (!vertexShaderBlob_)
	{
		if (engine)engine->Log(LogLevel::Error, "Failed to compile DepthOnly.VS.hlsl");
		throw std::runtime_error("Failed to compile DepthOnly.VS.hlsl");
	}

	/*-------------------------
		ルートパラメータの設定
	-------------------------*/

	D3D12_ROOT_PARAMETER rootParameter[1] = {};

	// VertexShader b0 
	rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameter[0].Descriptor.RegisterSpace = 0;
	rootParameter[0].Descriptor.ShaderRegister = 0;


	/*---------------------------------------
		ルートシグネチャのバイナリデータの生成
	---------------------------------------*/

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
		throw std::runtime_error("Failed to serialize root signature");
	}



	/*-------------------------
		ルートシグネチャの生成
	-------------------------*/

	// バイナリを元に生成
	hr = device->CreateRootSignature(0,
		signatureBlob_->GetBufferPointer(), signatureBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	if (FAILED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, "Failed to create root signature");
		throw std::runtime_error("Failed to create root signature");
	}



	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1]{};

	// POSITION 0 float4
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].InputSlot = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);


	// ラスタライズの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthClipEnable = true;


	// デプスの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.StencilEnable = false;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;


	// パイプステートの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.VS = { vertexShaderBlob_->GetBufferPointer(), vertexShaderBlob_->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { nullptr, 0 };
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;

	// RTVは書かない
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	graphicsPipelineStateDesc.NumRenderTargets = 0;

	// 書き込むDSVの情報
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 利用するトポロジ（形状）
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// 画面をうちこむ設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState_));
	if (FAILED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, "Failed to create graphics pipeline state");
		throw std::runtime_error("Failed to create graphics pipeline state");
	}
}

/// @brief コマンドリストに登録する
/// @param commandList 
void Detail::DepthOnlyPSO::Register(ID3D12GraphicsCommandList* commandList)
{
	commandList->SetPipelineState(graphicsPipelineState_.Get());
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
}