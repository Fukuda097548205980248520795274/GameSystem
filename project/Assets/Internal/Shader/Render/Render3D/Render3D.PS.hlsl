#include "Render3D.hlsli"
#include "../../LightCulling/LightData.hlsli"
#include "../../Camera/CameraData.hlsli"

// 出力ピクセルシェーダ
struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
};

// マテリアル
struct Material
{
	// 色
	float4 color;
	
	// uv行列
	float4x4 uvMatrix;
};
ConstantBuffer<Material> gMaterial : register(b0);

// カメラ情報
ConstantBuffer<Camera> gCamera : register(b1);

// ライト情報のバッファ
StructuredBuffer<PointLight> gPointLight : register(t1);
StructuredBuffer<SpotLight> gSpotLight : register(t2);

StructuredBuffer<uint> gTileLightIndexBuffer : register(t3);

// テクスチャ
Texture2D<float4> gTexture : register(t0);

// サンプラー
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	
	float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvMatrix); // UV座標を座標変換
	float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy); // テクスチャの色
	
	if (gMaterial.color.a * textureColor.a == 0.0f)
		discard; // a = 0は描画しない
	
	// アルベド色を計算
	float3 albedo = (gMaterial.color * textureColor).rgb;
	
	// 法線を正規化
	float3 normal = normalize(input.normal);
	
	// 最終的なライトの色
	float3 finalLight = float3(0.0f, 0.0f, 0.0f);

	// 現在のピクセル座標からタイルインデックスを計算
	uint2 pixelPos = (uint2) input.position.xy;
	uint2 tileID = pixelPos / TILE_SIZE;
	uint numTilesX = (uint) ceil(gCamera.screenDimensions.x / (float) TILE_SIZE);
	uint tileIndex = tileID.y * numTilesX + tileID.x;
	
	// バッファのオフセット計算
	uint bufferOffset = tileIndex * (MAX_LIGHTS_PER_TILE * 2 + 2);
	uint pointLightCount = gTileLightIndexBuffer[bufferOffset];
	uint spotLightOffset = bufferOffset + MAX_LIGHTS_PER_TILE + 1;
	uint spotLightCount = gTileLightIndexBuffer[spotLightOffset];

	// ポイントライトの計算
	for (uint i = 0; i < pointLightCount; ++i)
	{
		uint lightIndex = gTileLightIndexBuffer[bufferOffset + 1 + i];
		PointLight light = gPointLight[lightIndex];
		
		// ライト方向と距離を計算
		float3 lightDir = light.position - input.worldPosition;
		float dist = length(lightDir);
		lightDir /= dist;
		
		// ライトの影響範囲内であれば計算
		if (dist < light.radius)
		{
			float attenuation = pow(max(0.0f, 1.0f - (dist / light.radius)), light.decay);
			float nDotL = max(0.0f, dot(normal, lightDir));
			
			// ライトを加算
			finalLight += albedo * light.color.rgb * light.intensity * nDotL * attenuation;
		}
	}

	// スポットライトの計算
	for (uint j = 0; j < spotLightCount; ++j)
	{
		uint lightIndex = gTileLightIndexBuffer[spotLightOffset + 1 + j];
		SpotLight light = gSpotLight[lightIndex];
		
		// ライト方向と距離を計算
		float3 lightDir = light.position - input.worldPosition;
		float dist = length(lightDir);
		lightDir /= dist;
		
		// ライトの影響範囲内であれば計算
		if (dist < light.distance)
		{
			float attenuation = pow(max(0.0f, 1.0f - (dist / light.distance)), light.decay);
			float nDotL = max(0.0f, dot(normal, lightDir));
			
			// スポットライトの角度減衰
			float cosDir = dot(-lightDir, normalize(light.direction));
			float spotAttenuation = saturate((cosDir - light.cosAngle) / max(0.001f, (light.cosFalloffStart - light.cosAngle)));
			
			// ライトを加算
			finalLight += albedo * light.color.rgb * light.intensity * nDotL * attenuation * spotAttenuation;
		}
	}

	// アンビエント光を加算して出力
	float3 ambient = albedo * 0.1f;
	output.color = float4(ambient + finalLight, 1.0f);
	
	return output;
}