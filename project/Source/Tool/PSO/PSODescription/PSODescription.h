#pragma once
#include <d3d12.h>
#include <string>
#include <json.hpp>

namespace Detail
{
	// JSONライブラリの名前空間を簡略化
	using json = nlohmann::json;

	// ルートパラメータの種類
	enum class CustomRootParamType
	{
		CBV,
		SRV, 
		UAV, 
		DescriptorTable
	};

	enum class PSOType
	{ 
		Graphics, 
		Compute 
	};

	// エディタで選択するブレンドモード
	enum class EditorBlendMode
	{
		None,
		Normal,
		Add,
		Subtract,
		Multiply,
		Screen,
		Custom
	};

	// 詳細なブレンド設定データ
	struct CustomBlendDesc
	{
		bool blendEnable = false;
		D3D12_BLEND srcBlend = D3D12_BLEND_ONE;
		D3D12_BLEND destBlend = D3D12_BLEND_ZERO;
		D3D12_BLEND_OP blendOp = D3D12_BLEND_OP_ADD;
		D3D12_BLEND srcBlendAlpha = D3D12_BLEND_ONE;
		D3D12_BLEND destBlendAlpha = D3D12_BLEND_ZERO;
		D3D12_BLEND_OP blendOpAlpha = D3D12_BLEND_OP_ADD;
		UINT8 renderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	};

	/// @brief DescriptorTableの設定データ
	struct CustomDescriptorRange
	{
		D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		uint32_t numDescriptors = 1;
		uint32_t baseShaderRegister = 0;
		uint32_t registerSpace = 0;
		uint32_t offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	};

	// ルートパラメータの設定データ
	struct CustomRootParameter
	{
		CustomRootParamType type = CustomRootParamType::CBV;
		uint32_t shaderRegister = 0; // b0, t0, u0 などの 0 の部分
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;

		// DescriptorTableの場合の設定
		std::vector<CustomDescriptorRange> descriptorRanges;
	};

	// 静的サンプラーの設定データ
	struct CustomStaticSampler
	{
		uint32_t shaderRegister = 0; // s0 などの 0 の部分
		D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		D3D12_TEXTURE_ADDRESS_MODE addressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		D3D12_TEXTURE_ADDRESS_MODE addressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		D3D12_TEXTURE_ADDRESS_MODE addressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		D3D12_COMPARISON_FUNC comparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
	};

	// 入力レイアウトの設定データ
	struct CustomInputElement
	{
		std::string semanticName = "POSITION";
		uint32_t semanticIndex = 0;
		DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		uint32_t inputSlot = 0;
		uint32_t alignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		D3D12_INPUT_CLASSIFICATION inputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		uint32_t instanceDataStepRate = 0;
	};

	// エディタで編集対象とするステート設定データ
	struct PSODescription
	{
		// PSOの種別
		PSOType type = PSOType::Graphics;

		// 頂点シェーダ
		std::wstring vsPath = L"./Assets/Shader/Render3D.VS.hlsl";
		std::string  vsEntryPoint = "main";
		std::wstring  vsTarget = L"vs_6_0";

		// ピクセルシェーダ
		std::wstring psPath = L"./Assets/Shader/Render3D.PS.hlsl";
		std::string  psEntryPoint = "main";
		std::wstring  psTarget = L"ps_6_0";

		// コンピュートシェーダ
		std::wstring csPath = L"./Assets/Shader/Compute.CS.hlsl";
		std::string  csEntryPoint = "main";
		std::wstring csTarget = L"cs_6_0";

		// ラスタライザステート
		D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
		D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
		bool frontCounterClockwise = false;

		// デプスステンシルステート
		bool depthEnable = true;
		D3D12_DEPTH_WRITE_MASK depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		// ブレンドステート
		EditorBlendMode blendMode = EditorBlendMode::None;

		/// @brief カスタムブレンド設定
		CustomBlendDesc customBlend;

		// トポロジ
		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// ルートパラメータと静的サンプラーの設定
		std::vector<CustomRootParameter> rootParameters;
		std::vector<CustomStaticSampler> staticSamplers;

		// インプットレイアウトの設定
		std::vector<CustomInputElement> inputLayouts;
	};


	/// @brief CustomDescriptorRangeをJSONに変換する
	/// @param j 
	/// @param range 
	inline void ToJson(json& j, const CustomDescriptorRange& range)
	{
		j["rangeType"] = static_cast<int>(range.rangeType);
		j["numDescriptors"] = range.numDescriptors;
		j["baseShaderRegister"] = range.baseShaderRegister;
		j["registerSpace"] = range.registerSpace;
		j["offsetInDescriptorsFromTableStart"] = range.offsetInDescriptorsFromTableStart;
	}

	/// @brief CustomRootParameterをJSONに変換する
	/// @param j 
	/// @param param 
	inline void ToJson(json& j, const CustomRootParameter& param)
	{
		j["type"] = static_cast<int>(param.type);
		j["shaderRegister"] = param.shaderRegister;
		j["visibility"] = static_cast<int>(param.visibility);
		
		j["descriptorRanges"] = json::array();
		for (const auto& range : param.descriptorRanges)
		{
			json rangeJson;
			ToJson(rangeJson, range);
			j["descriptorRanges"].push_back(rangeJson);
		}
	}

	/// @brief CustomStaticSamplerをJSONに変換する
	/// @param j 
	/// @param sampler 
	inline void ToJson(json& j, const CustomStaticSampler& sampler)
	{
		j["shaderRegister"] = sampler.shaderRegister;
		j["filter"] = static_cast<int>(sampler.filter);
		j["addressU"] = static_cast<int>(sampler.addressU);
		j["addressV"] = static_cast<int>(sampler.addressV);
		j["addressW"] = static_cast<int>(sampler.addressW);
		j["comparisonFunc"] = static_cast<int>(sampler.comparisonFunc);
		j["visibility"] = static_cast<int>(sampler.visibility);
	}

	/// @brief CustomInputElementをJSONに変換する
	/// @param j 
	/// @param inputElement 
	inline void ToJson(json& j, const CustomInputElement& inputElement)
	{
		j["semanticName"] = inputElement.semanticName;
		j["semanticIndex"] = inputElement.semanticIndex;
		j["format"] = static_cast<int>(inputElement.format);
		j["inputSlot"] = inputElement.inputSlot;
		j["alignedByteOffset"] = inputElement.alignedByteOffset;
		j["inputSlotClass"] = static_cast<int>(inputElement.inputSlotClass);
		j["instanceDataStepRate"] = inputElement.instanceDataStepRate;
	}

	/// @brief CustomBlendDescをJSONに変換する
	/// @param j 
	/// @param blendDesc 
	inline void ToJson(json& j, const CustomBlendDesc& blendDesc)
	{
		j["blendEnable"] = blendDesc.blendEnable;
		j["srcBlend"] = static_cast<int>(blendDesc.srcBlend);
		j["destBlend"] = static_cast<int>(blendDesc.destBlend);
		j["blendOp"] = static_cast<int>(blendDesc.blendOp);
		j["srcBlendAlpha"] = static_cast<int>(blendDesc.srcBlendAlpha);
		j["destBlendAlpha"] = static_cast<int>(blendDesc.destBlendAlpha);
		j["blendOpAlpha"] = static_cast<int>(blendDesc.blendOpAlpha);
		j["renderTargetWriteMask"] = blendDesc.renderTargetWriteMask;
	}

	/// @brief PSODescriptionをJSONに変換する
	/// @param j 
	/// @param desc 
	inline void ToJson(json& j, const PSODescription& desc)
	{
		j["type"] = static_cast<int>(desc.type);

		j["vsPath"] = desc.vsPath;
		j["vsEntryPoint"] = desc.vsEntryPoint;
		j["vsTarget"] = desc.vsTarget;

		j["psPath"] = desc.psPath;
		j["psEntryPoint"] = desc.psEntryPoint;
		j["psTarget"] = desc.psTarget;

		j["csPath"] = desc.csPath;
		j["csEntryPoint"] = desc.csEntryPoint;
		j["csTarget"] = desc.csTarget;

		j["cullMode"] = static_cast<int>(desc.cullMode);
		j["fillMode"] = static_cast<int>(desc.fillMode);
		j["frontCounterClockwise"] = desc.frontCounterClockwise;

		j["depthEnable"] = desc.depthEnable;
		j["depthWriteMask"] = static_cast<int>(desc.depthWriteMask);
		j["depthFunc"] = static_cast<int>(desc.depthFunc);

		j["blendMode"] = static_cast<int>(desc.blendMode);
		ToJson(j["customBlend"], desc.customBlend);

		j["primitiveTopology"] = static_cast<int>(desc.primitiveTopology);

		// ルートパラメータの書き込み
		j["rootParameters"] = json::array();
		for (const auto& param : desc.rootParameters)
		{
			json paramJson;
			ToJson(paramJson, param);
			j["rootParameters"].push_back(paramJson);
		}

		// 静的サンプラーの書き込み
		j["staticSamplers"] = json::array();
		for (const auto& sampler : desc.staticSamplers)
		{
			json samplerJson;
			ToJson(samplerJson, sampler);
			j["staticSamplers"].push_back(samplerJson);
		}

		// インプットレイアウトの書き込み
		j["inputLayouts"] = json::array();
		for (const auto& inputElement : desc.inputLayouts)
		{
			json inputJson;
			ToJson(inputJson, inputElement);
			j["inputLayouts"].push_back(inputJson);
		}
	}

	/// @brief JSONからCustomDescriptorRangeに変換する
	/// @param j 
	/// @param range 
	inline void FromJson(const json& j, CustomDescriptorRange& range)
	{
		range.rangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(j.value("rangeType", static_cast<int>(range.rangeType)));
		range.numDescriptors = j.value("numDescriptors", range.numDescriptors);
		range.baseShaderRegister = j.value("baseShaderRegister", range.baseShaderRegister);
		range.registerSpace = j.value("registerSpace", range.registerSpace);
		range.offsetInDescriptorsFromTableStart = j.value("offsetInDescriptorsFromTableStart", range.offsetInDescriptorsFromTableStart);
	}

	/// @brief JSONからCustomRootParameterに変換する
	/// @param j 
	/// @param param 
	inline void FromJson(const json& j, CustomRootParameter& param)
	{
		param.type = static_cast<CustomRootParamType>(j.value("type", static_cast<int>(param.type)));
		param.shaderRegister = j.value("shaderRegister", param.shaderRegister);
		param.visibility = static_cast<D3D12_SHADER_VISIBILITY>(j.value("visibility", static_cast<int>(param.visibility)));
		
		// DescriptorRangesの読み込み
		if (j.contains("descriptorRanges") && j["descriptorRanges"].is_array())
		{
			param.descriptorRanges.clear();
			for (const auto& item : j["descriptorRanges"])
			{
				CustomDescriptorRange range;
				FromJson(item, range);
				param.descriptorRanges.push_back(range);
			}
		}
	}

	/// @brief JSONからCustomStaticSamplerに変換する
	/// @param j 
	/// @param sampler 
	inline void FromJson(const json& j, CustomStaticSampler& sampler)
	{
		sampler.shaderRegister = j.value("shaderRegister", sampler.shaderRegister);
		sampler.filter = static_cast<D3D12_FILTER>(j.value("filter", static_cast<int>(sampler.filter)));
		sampler.addressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(j.value("addressU", static_cast<int>(sampler.addressU)));
		sampler.addressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(j.value("addressV", static_cast<int>(sampler.addressV)));
		sampler.addressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(j.value("addressW", static_cast<int>(sampler.addressW)));
		sampler.comparisonFunc = static_cast<D3D12_COMPARISON_FUNC>(j.value("comparisonFunc", static_cast<int>(sampler.comparisonFunc)));
		sampler.visibility = static_cast<D3D12_SHADER_VISIBILITY>(j.value("visibility", static_cast<int>(sampler.visibility)));
	}

	/// @brief JSONからCustomInputElementに変換する
	/// @param j 
	/// @param inputElement 
	inline void FromJson(const json& j, CustomInputElement& inputElement)
	{
		inputElement.semanticName = j.value("semanticName", inputElement.semanticName);
		inputElement.semanticIndex = j.value("semanticIndex", inputElement.semanticIndex);
		inputElement.format = static_cast<DXGI_FORMAT>(j.value("format", static_cast<int>(inputElement.format)));
		inputElement.inputSlot = j.value("inputSlot", inputElement.inputSlot);
		inputElement.alignedByteOffset = j.value("alignedByteOffset", inputElement.alignedByteOffset);
		inputElement.inputSlotClass = static_cast<D3D12_INPUT_CLASSIFICATION>(j.value("inputSlotClass", static_cast<int>(inputElement.inputSlotClass)));
		inputElement.instanceDataStepRate = j.value("instanceDataStepRate", inputElement.instanceDataStepRate);
	}

	/// @brief JSONからCustomBlendDescに変換する
	/// @param j 
	/// @param blendDesc 
	inline void FromJson(const json& j, CustomBlendDesc& blendDesc)
	{
		blendDesc.blendEnable = j.value("blendEnable", blendDesc.blendEnable);
		blendDesc.srcBlend = static_cast<D3D12_BLEND>(j.value("srcBlend", static_cast<int>(blendDesc.srcBlend)));
		blendDesc.destBlend = static_cast<D3D12_BLEND>(j.value("destBlend", static_cast<int>(blendDesc.destBlend)));
		blendDesc.blendOp = static_cast<D3D12_BLEND_OP>(j.value("blendOp", static_cast<int>(blendDesc.blendOp)));
		blendDesc.srcBlendAlpha = static_cast<D3D12_BLEND>(j.value("srcBlendAlpha", static_cast<int>(blendDesc.srcBlendAlpha)));
		blendDesc.destBlendAlpha = static_cast<D3D12_BLEND>(j.value("destBlendAlpha", static_cast<int>(blendDesc.destBlendAlpha)));
		blendDesc.blendOpAlpha = static_cast<D3D12_BLEND_OP>(j.value("blendOpAlpha", static_cast<int>(blendDesc.blendOpAlpha)));
		blendDesc.renderTargetWriteMask = j.value("renderTargetWriteMask", blendDesc.renderTargetWriteMask);
	}

	/// @brief JSONからPSODescriptionに変換する
	/// @param j 
	/// @param desc 
	inline void FromJson(const json& j, PSODescription& desc)
	{
		desc.type = static_cast<PSOType>(j.value("type", static_cast<int>(desc.type)));

		desc.vsPath = j.value("vsPath", desc.vsPath);
		desc.vsEntryPoint = j.value("vsEntryPoint", desc.vsEntryPoint);
		desc.vsTarget = j.value("vsTarget", desc.vsTarget);

		desc.psPath = j.value("psPath", desc.psPath);
		desc.psEntryPoint = j.value("psEntryPoint", desc.psEntryPoint);
		desc.psTarget = j.value("psTarget", desc.psTarget);

		desc.csPath = j.value("csPath", desc.csPath);
		desc.csEntryPoint = j.value("csEntryPoint", desc.csEntryPoint);
		desc.csTarget = j.value("csTarget", desc.csTarget);

		desc.cullMode = static_cast<D3D12_CULL_MODE>(j.value("cullMode", static_cast<int>(desc.cullMode)));
		desc.fillMode = static_cast<D3D12_FILL_MODE>(j.value("fillMode", static_cast<int>(desc.fillMode)));

		desc.frontCounterClockwise = j.value("frontCounterClockwise", desc.frontCounterClockwise);

		desc.depthEnable = j.value("depthEnable", desc.depthEnable);
		desc.depthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(j.value("depthWriteMask", static_cast<int>(desc.depthWriteMask)));
		desc.depthFunc = static_cast<D3D12_COMPARISON_FUNC>(j.value("depthFunc", static_cast<int>(desc.depthFunc)));

		desc.blendMode = static_cast<EditorBlendMode>(j.value("blendMode", static_cast<int>(desc.blendMode)));
		FromJson(j.value("customBlend", json::object()), desc.customBlend);

		desc.primitiveTopology = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(j.value("primitiveTopology", static_cast<int>(desc.primitiveTopology)));

		// ルートパラメータの読み込み
		if (j.contains("rootParameters") && j["rootParameters"].is_array())
		{
			desc.rootParameters.clear();
			for (const auto& item : j["rootParameters"])
			{
				CustomRootParameter param;
				FromJson(item, param);
				desc.rootParameters.push_back(param);
			}
		}

		// 静的サンプラーの読み込み
		if (j.contains("staticSamplers") && j["staticSamplers"].is_array())
		{
			desc.staticSamplers.clear();
			for (const auto& item : j["staticSamplers"])
			{
				CustomStaticSampler sampler;
				FromJson(item, sampler);
				desc.staticSamplers.push_back(sampler);
			}
		}

		// インプットレイアウトの読み込み
		if (j.contains("inputLayouts") && j["inputLayouts"].is_array())
		{
			desc.inputLayouts.clear();
			for (const auto& item : j["inputLayouts"])
			{
				CustomInputElement inputElement;
				FromJson(item, inputElement);
				desc.inputLayouts.push_back(inputElement);
			}
		}
	}
}