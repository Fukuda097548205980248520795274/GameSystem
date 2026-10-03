#include "SwapChainCopyPSO.h"
#include <cassert>
#include "Engine.h"

/// @brief 初期化
/// @param device 
/// @param compiler 
void Detail::SwapChainCopyPSO::Initialize(ID3D12Device* device, ShaderCompiler* compiler)
{
	// nullptrチェック
	assert(device);
	assert(compiler);

	// エンジンのインスタンスを取得する
	auto engine = Engine::GetInstance();

	// ピクセルシェーダのバイナリデータを取得する
	vertexShaderBlob_ = compiler->Compile(L"./Assets/Shader/Fullscreen/Fullscreen.VS.hlsl", L"vs_6_0");
	if (!vertexShaderBlob_) {
		if (engine) engine->Log(LogLevel::Error, "Fullscreen.VS.hlslのコンパイルに失敗しました");
		throw std::runtime_error("Fullscreen.VS.hlslのコンパイルに失敗しました");
	}

	// ピクセルシェーダのバイナリデータを取得する
	pixelShaderBlob_ = compiler->Compile(L"./Assets/Shader/SwapChainCopy/SwapChainCopy.PS.hlsl", L"ps_6_0");
	if (!pixelShaderBlob_) {
		if (engine) engine->Log(LogLevel::Error, "SwapChainCopy.PS.hlslのコンパイルに失敗しました");
		throw std::runtime_error("SwapChainCopy.PS.hlslのコンパイルに失敗しました");
	}


	/*------------------------------
		ディスクリプタレンジの設定
	------------------------------*/

	// SRV t0 テクスチャ
	D3D12_DESCRIPTOR_RANGE descriptorTexture[1];
	descriptorTexture[0].BaseShaderRegister = 0;
	descriptorTexture[0].RegisterSpace = 0;
	descriptorTexture[0].NumDescriptors = 1;
	descriptorTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



	/*-------------------------
		ルートパラメータの設定
	-------------------------*/

	D3D12_ROOT_PARAMETER rootParameter[1];

	// DescriptorTable PixelShader t0 テクスチャ
	rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameter[0].DescriptorTable.pDescriptorRanges = descriptorTexture;
	rootParameter[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorTexture);


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
		if (engine) engine->Log(LogLevel::Error, reinterpret_cast<char*>(errorBlob_->GetBufferPointer()));
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

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = nullptr;
	inputLayoutDesc.NumElements = 0;


	/*-------------------------
		ブレンドステートの設定
	-------------------------*/

	D3D12_BLEND_DESC blendDesc{};

	// 全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


	/*-----------------------------
		ラスタライザステートの設定
	-----------------------------*/

	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// 裏面をカリングする
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;


	/*----------------------
		深度ステートの設定
	----------------------*/

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	// Depthは無効にする
	depthStencilDesc.DepthEnable = false;


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
	if (!SUCCEEDED(hr)) {
		if (engine) engine->Log(LogLevel::Error, "PSOの生成に失敗しました");
		throw std::runtime_error("PSOの生成に失敗しました");
	}
	if (engine)engine->Log(LogLevel::Info, "SwapChainCopyPSO生成");
}

/// @brief コマンドリストに登録する
/// @param commandList 
void Detail::SwapChainCopyPSO::Register(ID3D12GraphicsCommandList* commandList)
{
	// nullptrチェック
	assert(commandList);

	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(graphicsPipelineState_.Get());
}