#pragma once
#include <d3d12.h>
#include <string>
#include <json.hpp>
#include "ECS/ComponentECS/ComponentECS.h"

// JSONライブラリの名前空間を簡略化
using json = nlohmann::json;

namespace Detail
{
	// ルートパラメータの種類
	enum class CustomRootParamType { CBV, SRV, UAV, DescriptorTable };

	// ルートパラメータの設定データ
	struct CustomRootParameter
	{
		CustomRootParamType type = CustomRootParamType::CBV;
		uint32_t shaderRegister = 0; // b0, t0, u0 などの 0 の部分
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;

		// DescriptorTableの場合の設定（簡略化のためレンジは1つとする）
		D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		uint32_t numDescriptors = 1;
	};

	// 静的サンプラーの設定データ
	struct CustomStaticSampler
	{
		uint32_t shaderRegister = 0; // s0 などの 0 の部分
		D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		D3D12_TEXTURE_ADDRESS_MODE addressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		D3D12_TEXTURE_ADDRESS_MODE addressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		D3D12_TEXTURE_ADDRESS_MODE addressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
	};

	// エディタで編集対象とするステート設定データ
	struct PSODescription
	{
		// シェーダファイル情報
		std::wstring vsPath = L"./Assets/Shader/Render3D.VS.hlsl";
		std::string  vsEntryPoint = "main";
		std::wstring  vsTarget = L"vs_6_0";

		std::wstring psPath = L"./Assets/Shader/Render3D.PS.hlsl";
		std::string  psEntryPoint = "main";
		std::wstring  psTarget = L"ps_6_0";

		// ラスタライザステート
		D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
		D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
		bool frontCounterClockwise = false;

		// デプスステンシルステート
		bool depthEnable = true;
		D3D12_DEPTH_WRITE_MASK depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		// ブレンドステート
		BlendMode blendMode = BlendMode::None;

		// トポロジ
		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// ルートパラメータと静的サンプラーの設定
		std::vector<CustomRootParameter> rootParameters;
		std::vector<CustomStaticSampler> staticSamplers;
	};

	/// @brief CustomRootParameterをJSONに変換する
	/// @param j 
	/// @param param 
	inline void ToJson(json& j, const CustomRootParameter& param)
	{
		j["type"] = static_cast<int>(param.type);
		j["shaderRegister"] = param.shaderRegister;
		j["visibility"] = static_cast<int>(param.visibility);
		j["rangeType"] = static_cast<int>(param.rangeType);
		j["numDescriptors"] = param.numDescriptors;
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
		j["visibility"] = static_cast<int>(sampler.visibility);
	}

	/// @brief PSODescriptionをJSONに変換する
	/// @param j 
	/// @param desc 
	inline void ToJson(json& j, const PSODescription& desc)
	{
		j["vsPath"] = desc.vsPath;
		j["vsEntryPoint"] = desc.vsEntryPoint;
		j["vsTarget"] = desc.vsTarget;

		j["psPath"] = desc.psPath;
		j["psEntryPoint"] = desc.psEntryPoint;
		j["psTarget"] = desc.psTarget;

		j["cullMode"] = static_cast<int>(desc.cullMode);
		j["fillMode"] = static_cast<int>(desc.fillMode);
		j["frontCounterClockwise"] = desc.frontCounterClockwise;

		j["depthEnable"] = desc.depthEnable;
		j["depthWriteMask"] = static_cast<int>(desc.depthWriteMask);
		j["depthFunc"] = static_cast<int>(desc.depthFunc);

		j["blendMode"] = static_cast<int>(desc.blendMode);

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
	}

	/// @brief JSONからCustomRootParameterに変換する
	/// @param j 
	/// @param param 
	inline void FromJson(const json& j, CustomRootParameter& param)
	{
		param.type = static_cast<CustomRootParamType>(j.value("type", static_cast<int>(param.type)));
		param.shaderRegister = j.value("shaderRegister", param.shaderRegister);
		param.visibility = static_cast<D3D12_SHADER_VISIBILITY>(j.value("visibility", static_cast<int>(param.visibility)));
		param.rangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(j.value("rangeType", static_cast<int>(param.rangeType)));
		param.numDescriptors = j.value("numDescriptors", param.numDescriptors);
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
		sampler.visibility = static_cast<D3D12_SHADER_VISIBILITY>(j.value("visibility", static_cast<int>(sampler.visibility)));
	}

	/// @brief JSONからPSODescriptionに変換する
	/// @param j 
	/// @param desc 
	inline void FromJson(const json& j, PSODescription& desc)
	{
		desc.vsPath = j.value("vsPath", desc.vsPath);
		desc.vsEntryPoint = j.value("vsEntryPoint", desc.vsEntryPoint);
		desc.vsTarget = j.value("vsTarget", desc.vsTarget);

		desc.psPath = j.value("psPath", desc.psPath);
		desc.psEntryPoint = j.value("psEntryPoint", desc.psEntryPoint);
		desc.psTarget = j.value("psTarget", desc.psTarget);

		desc.cullMode = static_cast<D3D12_CULL_MODE>(j.value("cullMode", static_cast<int>(desc.cullMode)));
		desc.fillMode = static_cast<D3D12_FILL_MODE>(j.value("fillMode", static_cast<int>(desc.fillMode)));

		desc.frontCounterClockwise = j.value("frontCounterClockwise", desc.frontCounterClockwise);

		desc.depthEnable = j.value("depthEnable", desc.depthEnable);
		desc.depthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(j.value("depthWriteMask", static_cast<int>(desc.depthWriteMask)));
		desc.depthFunc = static_cast<D3D12_COMPARISON_FUNC>(j.value("depthFunc", static_cast<int>(desc.depthFunc)));

		desc.blendMode = static_cast<BlendMode>(j.value("blendMode", static_cast<int>(desc.blendMode)));

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
	}
}