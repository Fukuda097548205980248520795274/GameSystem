#include "Skybox.hlsli"

// 入力頂点
struct VertexShaderInput
{
    float4 position : POSITION0;
};

// 座標変換
struct Transformation
{
    float4x4 worldViewProjection;
};
ConstantBuffer<Transformation> gTransformation : register(b0);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // 座標変換
    output.position = mul(input.position, gTransformation.worldViewProjection).xyww;
    output.texcoord = input.position.xyz;
    
    return output;
}