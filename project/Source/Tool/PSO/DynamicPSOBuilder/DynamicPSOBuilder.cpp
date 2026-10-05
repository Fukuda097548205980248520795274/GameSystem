#include "DynamicPSOBuilder.h"
#include "Engine.h"

/// @brief 設定データから動的にPSOとRootSignatureを生成する
/// @param device 
/// @param compiler 
/// @param desc 
/// @param defaultRootSignature 
/// @param outPipelineState 
/// @return 
bool Detail::DynamicPSOBuilder::Build(ID3D12Device* device, ShaderCompiler* compiler, const PSODescription& desc, ID3D12RootSignature** outRootSignature, ID3D12PipelineState** outPipelineState)
{
	auto engine = Engine::GetInstance();


	/* ====================================================
	   1. ルートシグネチャの動的生成
	==================================================== */
	std::vector<D3D12_ROOT_PARAMETER> rootParams(desc.rootParameters.size());
	std::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges(desc.rootParameters.size()); // メモリを確保しておく

	for (size_t i = 0; i < desc.rootParameters.size(); ++i)
	{
		const auto& p = desc.rootParameters[i];
		rootParams[i].ShaderVisibility = p.visibility;

		if (p.type == CustomRootParamType::DescriptorTable)
		{
			descriptorRanges[i].RangeType = p.rangeType;
			descriptorRanges[i].NumDescriptors = p.numDescriptors;
			descriptorRanges[i].BaseShaderRegister = p.shaderRegister;
			descriptorRanges[i].RegisterSpace = 0;
			descriptorRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[i].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[i].DescriptorTable.pDescriptorRanges = &descriptorRanges[i];
		}
		else
		{
			if (p.type == CustomRootParamType::CBV) rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			if (p.type == CustomRootParamType::SRV) rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			if (p.type == CustomRootParamType::UAV) rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;

			rootParams[i].Descriptor.ShaderRegister = p.shaderRegister;
			rootParams[i].Descriptor.RegisterSpace = 0;
		}
	}

	// サンプラーの構築
	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers(desc.staticSamplers.size());
	for (size_t i = 0; i < desc.staticSamplers.size(); ++i)
	{
		const auto& s = desc.staticSamplers[i];
		samplers[i].Filter = s.filter;
		samplers[i].AddressU = s.addressU;
		samplers[i].AddressV = s.addressV;
		samplers[i].AddressW = s.addressW;
		samplers[i].MipLODBias = 0.0f;
		samplers[i].MaxAnisotropy = 1;
		samplers[i].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplers[i].MinLOD = 0.0f;
		samplers[i].MaxLOD = D3D12_FLOAT32_MAX;
		samplers[i].ShaderRegister = s.shaderRegister;
		samplers[i].RegisterSpace = 0;
		samplers[i].ShaderVisibility = s.visibility;
	}

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSigDesc.NumParameters = static_cast<UINT>(rootParams.size());
	rootSigDesc.pParameters = rootParams.data();
	rootSigDesc.NumStaticSamplers = static_cast<UINT>(samplers.size());
	rootSigDesc.pStaticSamplers = samplers.data();

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob, errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		if (engine) engine->Log(LogLevel::Error, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		return false;
	}

	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(outRootSignature));
	if (FAILED(hr)) return false;


	// 1. シェーダのコンパイル
	auto vsBlob = compiler->Compile(desc.vsPath.c_str(), desc.vsTarget.c_str());
	auto psBlob = compiler->Compile(desc.psPath.c_str(), desc.psTarget.c_str());

	if (!vsBlob || !psBlob) 
	{
		if (engine) engine->Log(LogLevel::Error, "Shader Compilation Failed!");
		return false; // コンパイルエラー時は生成を中断（前のPSOを維持するため）
	}

	// 2. インプットレイアウトの定義（必要に応じて設定可）
	D3D12_INPUT_ELEMENT_DESC inputElements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 3. パイプラインステート記述子の構築
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = *outRootSignature;
	psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	psoDesc.InputLayout = { inputElements, _countof(inputElements) };

	// ラスタライザステートの反映
	psoDesc.RasterizerState.CullMode = desc.cullMode;
	psoDesc.RasterizerState.FillMode = desc.fillMode;
	psoDesc.RasterizerState.FrontCounterClockwise = desc.frontCounterClockwise;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;

	// デプスステンシルステートの反映
	psoDesc.DepthStencilState.DepthEnable = desc.depthEnable;
	psoDesc.DepthStencilState.DepthWriteMask = desc.depthWriteMask;
	psoDesc.DepthStencilState.DepthFunc = desc.depthFunc;

	// ブレンドステートの反映 (既存の CreateBlendMode 活用)
	psoDesc.BlendState = CreateBlendMode(desc.blendMode);

	// レンダーターゲット＆フォーマット設定
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.PrimitiveTopologyType = desc.primitiveTopology;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 4. PSOの生成
	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(outPipelineState));
	return SUCCEEDED(hr);
}


/// @brief ブレンドモードを作成する
/// @param blendMode 
/// @return 
D3D12_BLEND_DESC Detail::DynamicPSOBuilder::CreateBlendMode(BlendMode blendMode)
{
	switch (blendMode)
	{
	case BlendMode::None:
	default:
		// 合成なし
		return CreateBlendNone();
		break;

	case BlendMode::Normal:
		// ノーマル合成
		return CreateBlendNormal();
		break;

	case BlendMode::Add:
		// 加算合成
		return CreateBlendAdd();
		break;

	case BlendMode::Subtract:
		// 減算合成
		return CreateBlendSubtract();
		break;

	case BlendMode::Multiply:
		// 乗算合成
		return CreateBlendMultiply();
		break;

	case BlendMode::Screen:
		// スクリーン合成
		return CreateBlendScreen();
		break;
	}
}

/// @brief ブレンドモード作成 : 合成なし
/// @return 
D3D12_BLEND_DESC Detail::DynamicPSOBuilder::CreateBlendNone()
{
	D3D12_BLEND_DESC blendDesc{};

	// 全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blendDesc;
}

/// @brief ブレンドモード作成 : ノーマル合成
/// @return 
D3D12_BLEND_DESC Detail::DynamicPSOBuilder::CreateBlendNormal()
{
	D3D12_BLEND_DESC blendDesc{};

	// RGBを書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

	return blendDesc;
}

/// @brief ブレンドモード作成 : 加算合成
/// @return 
D3D12_BLEND_DESC Detail::DynamicPSOBuilder::CreateBlendAdd()
{
	D3D12_BLEND_DESC blendDesc{};

	// RGBを書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

	return blendDesc;
}

/// @brief ブレンドモード作成 : 減算合成
/// @return 
D3D12_BLEND_DESC Detail::DynamicPSOBuilder::CreateBlendSubtract()
{
	D3D12_BLEND_DESC blendDesc{};

	// RGBを書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

	return blendDesc;
}

/// @brief ブレンドモード作成 : 乗算合成
/// @return 
D3D12_BLEND_DESC Detail::DynamicPSOBuilder::CreateBlendMultiply()
{
	D3D12_BLEND_DESC blendDesc{};

	// RGBを書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

	return blendDesc;
}

/// @brief ブレンドモード作成 : スクリーン合成
/// @return 
D3D12_BLEND_DESC Detail::DynamicPSOBuilder::CreateBlendScreen()
{
	D3D12_BLEND_DESC blendDesc{};

	// RGBを書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

	return blendDesc;
}