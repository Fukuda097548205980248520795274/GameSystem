#include "DepthOnly.hlsli"

// 入力頂点シェーダ
struct VertexShaderInput
{
    float4 position : POSITION0;
};

// 座標変換行列
struct TransformationMatrix
{
    float4x4 world;
    float4x4 worldViewProjection;
    float4x4 worldInverseTranspose;
};
StructuredBuffer<TransformationMatrix> gTransformation : register(t0);

VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    
    // 同次クリップ空間に変換
    output.position = mul(input.position, gTransformation[instanceID].worldViewProjection);
    
    return output;
}
