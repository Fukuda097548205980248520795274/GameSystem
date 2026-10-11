#include "Skybox.hlsli"

// 出力ピクセル
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// マテリアル
struct Material
{
    // 色
    float4 color;
};
ConstantBuffer<Material> gMateria : register(b0);

TextureCube<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // テクスチャのサンプリング
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = textureColor * gMateria.color;
    
    return output;
}