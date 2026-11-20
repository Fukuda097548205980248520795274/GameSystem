#include "Prefab3D.hlsli"

// 頂点シェーダ入力構造体
struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

// インスタンスデータ構造体
struct InstanceData
{
    matrix world;
    matrix worldViewProjection;
    matrix worldInverseTranspose;
    float4 color;
    matrix uvMatrix;
    uint textureIndex;
    uint3 pad;
};
StructuredBuffer<InstanceData> gInstanceData : register(t0);

VertexShaderOutput main(VertexShaderInput input , uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    
    // 同次クリップ空間に変換
    output.position = mul(input.position, gInstanceData[instanceID].worldViewProjection);
    
    // UV座標を渡す
    output.texcoord = input.texcoord;
    
    // ワールド空間での位置を計算
    output.worldPosition = mul(input.position, gInstanceData[instanceID].world).xyz;
    
    // 法線をワールド空間に変換し、正規化する
    output.normal = normalize(mul(float4(input.normal, 0.0f), gInstanceData[instanceID].worldInverseTranspose).xyz);

    // マテリアル情報を渡す
    output.color = gInstanceData[instanceID].color;
    output.uvTransform = gInstanceData[instanceID].uvMatrix;
    output.textureIndex = gInstanceData[instanceID].textureIndex;

    // インスタンスIDを渡す
    output.instanceID = instanceID;
    
    return output;
}
