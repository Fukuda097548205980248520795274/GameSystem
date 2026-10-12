#include "Render3D.hlsli"

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

// テクスチャ
Texture2D<float4> gTexture : register(t0);

// サンプラー
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // UV座標を座標変換する
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvMatrix);
    
    // テクスチャの色
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // a = 0は描画しない
    if (gMaterial.color.a * textureColor.a == 0.0f)
        discard;
    
    // 色
    output.color = gMaterial.color * textureColor;
    
    return output;
}