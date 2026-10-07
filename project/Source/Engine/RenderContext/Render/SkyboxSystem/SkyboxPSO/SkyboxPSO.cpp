#include "SkyboxPSO.h"
#include <cassert>
#include "Engine.h"

/// @brief 初期化
/// @param device 
/// @param vertexShaderBlob 
/// @param pixelShaderBlob 
/// @param log 
void Detail::SkyboxPSO::Initialize(ID3D12Device* device, ShaderCompiler* compiler)
{
	// nullptrチェック
	assert(device);
	assert(compiler);

	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	// ピクセルシェーダのバイナリデータを取得する
	vertexShaderBlob_ = compiler->Compile(L"./Assets/Shader/Skybox/Skybox.VS.hlsl", L"vs_6_0");
	if (!vertexShaderBlob_)
	{
		if (engine)engine->Log(LogLevel::Error, "Skybox.VS.hlslのコンパイルに失敗しました");
		throw std::runtime_error("Skybox.VS.hlslのコンパイルに失敗しました");
	}

	// ピクセルシェーダのバイナリデータを取得する
	pixelShaderBlob_ = compiler->Compile(L"./Assets/Shader/Skybox/Skybox.PS.hlsl", L"ps_6_0");
	if (!pixelShaderBlob_)
	{
		if (engine)engine->Log(LogLevel::Error, "Skybox.PS.hlslのコンパイルに失敗しました");
		throw std::runtime_error("Skybox.PS.hlslのコンパイルに失敗しました");
	}


	/*------------------------------
		ディスクリプタレンジの設定
	------------------------------*/

	D3D12_DESCRIPTOR_RANGE textureDescriptor[1];
	textureDescriptor[0].BaseShaderRegister = 0;
	textureDescriptor[0].RegisterSpace = 0;
	textureDescriptor[0].NumDescriptors = 1;
	textureDescriptor[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	textureDescriptor[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



	/*-------------------------
		ルートパラメータの設定
	-------------------------*/

	D3D12_ROOT_PARAMETER rootParameter[3];

	// CBV VertexShader b0
	rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameter[0].Descriptor.ShaderRegister = 0;
	rootParameter[0].Descriptor.RegisterSpace = 0;

	// CBV PixelShader b0
	rootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameter[1].Descriptor.ShaderRegister = 0;
	rootParameter[1].Descriptor.RegisterSpace = 0;

	// DescriptorTable PixelShader t0 
	rootParameter[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameter[2].DescriptorTable.pDescriptorRanges = textureDescriptor;
	rootParameter[2].DescriptorTable.NumDescriptorRanges = _countof(textureDescriptor);


	/*--------------------
		サンプラーの設定
	--------------------*/

	D3D12_STATIC_SAMPLER_DESC samplers[1] = {};

	// サンプラー
	samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[0].ShaderRegister = 0;
	samplers[0].RegisterSpace = 0;
	samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


	/*---------------------------------------
		ルートシグネチャのバイナリデータの生成
	---------------------------------------*/

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// ルートパラメータを設定する
	descriptionRootSignature.pParameters = rootParameter;
	descriptionRootSignature.NumParameters = _countof(rootParameter);

	// サンプラーを設定する
	descriptionRootSignature.pStaticSamplers = samplers;
	descriptionRootSignature.NumStaticSamplers = _countof(samplers);

	// シリアライズしてバイナリにする
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob_, &errorBlob_);

	// エラーのとき、情報を出力し停止させる
	if (FAILED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, reinterpret_cast<char*>(errorBlob_->GetBufferPointer()));
		throw std::runtime_error("ルートシグネチャの生成に失敗しました");
	}



	/*-------------------------
		ルートシグネチャの生成
	-------------------------*/

	// バイナリを元に生成
	hr = device->CreateRootSignature(0,
		signatureBlob_->GetBufferPointer(), signatureBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));


	/*----------------------------
		インプットレイアウトの設定
	----------------------------*/

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};

	// POSITION 0 float4
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);


	/*-------------------------
		ブレンドステートの設定
	-------------------------*/

	D3D12_BLEND_DESC blendDesc{};

	// 全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;


	/*-----------------------------
		ラスタライザステートの設定
	-----------------------------*/

	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// 表面をカリングする
	rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;

	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;


	/*----------------------
		深度ステートの設定
	----------------------*/

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	// Depthを有効化する
	depthStencilDesc.DepthEnable = true;

	// 書き込まない
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	// 比較関数　近ければ描画する
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;


	/*------------------
		PSOを生成する
	------------------*/

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = { vertexShaderBlob_->GetBufferPointer(), vertexShaderBlob_->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob_->GetBufferPointer(), pixelShaderBlob_->GetBufferSize() };
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;

	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// 書き込むDSVの情報
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 利用するトポロジ（形状）
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// 画面をうちこむ設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 生成
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState_));
	if (!SUCCEEDED(hr))
	{
		if (engine)engine->Log(LogLevel::Error, "PSOの生成に失敗しました");
		throw std::runtime_error("PSOの生成に失敗しました");
	}
	if (engine)engine->Log(LogLevel::Info, "SkyboxPSO生成");
}