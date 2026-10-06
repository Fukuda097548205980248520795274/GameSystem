#include "DynamicPSOBuilder.h"
#include "Engine.h"
#include "Func/ConvertString/ConvertString.h"

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


	// ルートパラメータの構築
	std::vector<D3D12_ROOT_PARAMETER> rootParams(desc.rootParameters.size());
	
	// DescriptorTableの範囲を保持するためのベクターを作成
	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> allDescriptorRanges(desc.rootParameters.size());

	for (size_t i = 0; i < desc.rootParameters.size(); ++i)
	{
		const auto& p = desc.rootParameters[i];
		rootParams[i].ShaderVisibility = p.visibility;

		// DescriptorTableの場合の設定
		if (p.type == CustomRootParamType::DescriptorTable)
		{
			auto& ranges = allDescriptorRanges[i];
			ranges.resize(p.descriptorRanges.size());

			for (size_t j = 0; j < p.descriptorRanges.size(); ++j)
			{
				ranges[j].RangeType = p.descriptorRanges[j].rangeType;
				ranges[j].NumDescriptors = p.descriptorRanges[j].numDescriptors;
				ranges[j].BaseShaderRegister = p.descriptorRanges[j].baseShaderRegister;
				ranges[j].RegisterSpace = p.descriptorRanges[j].registerSpace;
				ranges[j].OffsetInDescriptorsFromTableStart = p.descriptorRanges[j].offsetInDescriptorsFromTableStart;
			}

			rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[i].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(ranges.size());
			rootParams[i].DescriptorTable.pDescriptorRanges = ranges.empty() ? nullptr : ranges.data();
		}
		else
		{
			// DescriptorTable以外の設定

			// ルートパラメータのタイプを設定
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
		samplers[i].ComparisonFunc = s.comparisonFunc;
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


	if (desc.type == PSOType::Compute)
	{
		// --- コンピュートシェーダのコンパイル ---
		auto csResult = compiler->CompileFile(desc.csPath, desc.csTarget.c_str(), ConvertString(desc.csEntryPoint).c_str());
		if (!csResult.success)
		{
			if (engine) engine->Log(LogLevel::Error, "Compute Shader Compilation Failed: " + csResult.errorMessage);
			return false;
		}

		// --- Compute PSO Desc の構築 ---
		D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc{};
		computePsoDesc.pRootSignature = *outRootSignature;
		computePsoDesc.CS = { csResult.blob->GetBufferPointer(), csResult.blob->GetBufferSize() };
		computePsoDesc.NodeMask = 0;

		HRESULT hr = device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(outPipelineState));
		return SUCCEEDED(hr);
	}
	else
	{
		// 1. シェーダのコンパイル
		auto vsResult = compiler->CompileFile(desc.vsPath, desc.vsTarget.c_str(), ConvertString(desc.vsEntryPoint).c_str());
		auto psResult = compiler->CompileFile(desc.psPath, desc.psTarget.c_str(), ConvertString(desc.psEntryPoint).c_str());

		// PSOの生成を中止するかどうかの判定
		if (!vsResult.success || !psResult.success)
		{
			if (engine)
			{
				engine->Log(LogLevel::Error, "Shader Compilation Failed!");
				if (!vsResult.success) engine->Log(LogLevel::Error, "VS Error: " + vsResult.errorMessage);
				if (!psResult.success) engine->Log(LogLevel::Error, "PS Error: " + psResult.errorMessage);
			}

			// PSOの生成を中止
			return false;
		}

		// 2. インプットレイアウトの定義（エディタからの設定を反映）
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements(desc.inputLayouts.size());
		for (size_t i = 0; i < desc.inputLayouts.size(); ++i)
		{
			const auto& elem = desc.inputLayouts[i];

			// elem.semanticName は builder 関数が終わるまで有効なので .c_str() で渡して問題ありません
			inputElements[i].SemanticName = elem.semanticName.c_str();
			inputElements[i].SemanticIndex = elem.semanticIndex;
			inputElements[i].Format = elem.format;
			inputElements[i].InputSlot = elem.inputSlot;
			inputElements[i].AlignedByteOffset = elem.alignedByteOffset;
			inputElements[i].InputSlotClass = elem.inputSlotClass;
			inputElements[i].InstanceDataStepRate = elem.instanceDataStepRate;
		}

		// 3. パイプラインステート記述子の構築
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = *outRootSignature;
		psoDesc.VS = { vsResult.blob->GetBufferPointer(), vsResult.blob->GetBufferSize() };
		psoDesc.PS = { psResult.blob->GetBufferPointer(), psResult.blob->GetBufferSize() };

		// 動的に作成した inputElements を割り当て
		psoDesc.InputLayout = { inputElements.data(), static_cast<UINT>(inputElements.size()) };

		// ラスタライザステートの反映
		psoDesc.RasterizerState.CullMode = desc.cullMode;
		psoDesc.RasterizerState.FillMode = desc.fillMode;
		psoDesc.RasterizerState.FrontCounterClockwise = desc.frontCounterClockwise;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;

		// デプスステンシルステートの反映
		psoDesc.DepthStencilState.DepthEnable = desc.depthEnable;
		psoDesc.DepthStencilState.DepthWriteMask = desc.depthWriteMask;
		psoDesc.DepthStencilState.DepthFunc = desc.depthFunc;

		// ブレンドステートの反映
		psoDesc.BlendState = CreateBlendMode(desc);

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
}


/// @brief ブレンドモードのプリセットを取得する
/// @param blendMode 
/// @return 
Detail::CustomBlendDesc Detail::DynamicPSOBuilder::GetPresetBlendDesc(EditorBlendMode blendMode)
{
	CustomBlendDesc cb{};
	switch (blendMode)
	{
	case EditorBlendMode::Normal:
		cb.blendEnable = true;
		cb.srcBlend = D3D12_BLEND_SRC_ALPHA; cb.destBlend = D3D12_BLEND_INV_SRC_ALPHA; cb.blendOp = D3D12_BLEND_OP_ADD;
		cb.srcBlendAlpha = D3D12_BLEND_ONE; cb.destBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA; cb.blendOpAlpha = D3D12_BLEND_OP_ADD;
		break;
	case EditorBlendMode::Add:
		cb.blendEnable = true;
		cb.srcBlend = D3D12_BLEND_SRC_ALPHA; cb.destBlend = D3D12_BLEND_ONE; cb.blendOp = D3D12_BLEND_OP_ADD;
		cb.srcBlendAlpha = D3D12_BLEND_ONE; cb.destBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA; cb.blendOpAlpha = D3D12_BLEND_OP_ADD;
		break;
	case EditorBlendMode::Subtract:
		cb.blendEnable = true;
		cb.srcBlend = D3D12_BLEND_SRC_ALPHA; cb.destBlend = D3D12_BLEND_ONE; cb.blendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		cb.srcBlendAlpha = D3D12_BLEND_ONE; cb.destBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA; cb.blendOpAlpha = D3D12_BLEND_OP_ADD;
		break;
	case EditorBlendMode::Multiply:
		cb.blendEnable = true;
		cb.srcBlend = D3D12_BLEND_ZERO; cb.destBlend = D3D12_BLEND_SRC_COLOR; cb.blendOp = D3D12_BLEND_OP_ADD;
		cb.srcBlendAlpha = D3D12_BLEND_ONE; cb.destBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA; cb.blendOpAlpha = D3D12_BLEND_OP_ADD;
		break;
	case EditorBlendMode::Screen:
		cb.blendEnable = true;
		cb.srcBlend = D3D12_BLEND_INV_DEST_COLOR; cb.destBlend = D3D12_BLEND_ONE; cb.blendOp = D3D12_BLEND_OP_ADD;
		cb.srcBlendAlpha = D3D12_BLEND_ONE; cb.destBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA; cb.blendOpAlpha = D3D12_BLEND_OP_ADD;
		break;
	case EditorBlendMode::None:
	default:
		cb.blendEnable = false;
		break;
	}
	return cb;
}


/// @brief ブレンドモードを作成する
/// @param blendMode 
/// @return 
D3D12_BLEND_DESC Detail::DynamicPSOBuilder::CreateBlendMode(EditorBlendMode blendMode)
{
	switch (blendMode)
	{
	case EditorBlendMode::None:
	default:
		// 合成なし
		return CreateBlendNone();
		break;

	case EditorBlendMode::Normal:
		// ノーマル合成
		return CreateBlendNormal();
		break;

	case EditorBlendMode::Add:
		// 加算合成
		return CreateBlendAdd();
		break;

	case EditorBlendMode::Subtract:
		// 減算合成
		return CreateBlendSubtract();
		break;

	case EditorBlendMode::Multiply:
		// 乗算合成
		return CreateBlendMultiply();
		break;

	case EditorBlendMode::Screen:
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

/// @brief ブレンドモード作成 : カスタム合成
/// @param desc 
/// @return 
D3D12_BLEND_DESC Detail::DynamicPSOBuilder::CreateBlendMode(const PSODescription& desc)
{
	if (desc.blendMode == EditorBlendMode::Custom)
	{
		D3D12_BLEND_DESC blendDesc{};
		blendDesc.RenderTarget[0].BlendEnable = desc.customBlend.blendEnable;
		blendDesc.RenderTarget[0].SrcBlend = desc.customBlend.srcBlend;
		blendDesc.RenderTarget[0].DestBlend = desc.customBlend.destBlend;
		blendDesc.RenderTarget[0].BlendOp = desc.customBlend.blendOp;
		blendDesc.RenderTarget[0].SrcBlendAlpha = desc.customBlend.srcBlendAlpha;
		blendDesc.RenderTarget[0].DestBlendAlpha = desc.customBlend.destBlendAlpha;
		blendDesc.RenderTarget[0].BlendOpAlpha = desc.customBlend.blendOpAlpha;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = desc.customBlend.renderTargetWriteMask;
		return blendDesc;
	}

	// 既存プリセット処理
	switch (desc.blendMode)
	{
	case EditorBlendMode::Normal:   return CreateBlendNormal();
	case EditorBlendMode::Add:      return CreateBlendAdd();
	case EditorBlendMode::Subtract: return CreateBlendSubtract();
	case EditorBlendMode::Multiply: return CreateBlendMultiply();
	case EditorBlendMode::Screen:   return CreateBlendScreen();
	case EditorBlendMode::None:
	default:                  return CreateBlendNone();
	}
}