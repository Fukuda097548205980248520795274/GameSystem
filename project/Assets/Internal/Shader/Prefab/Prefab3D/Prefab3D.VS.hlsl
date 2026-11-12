#include "Prefab3D.hlsli"

// 頂点シェーダ入力構造体
struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

// 座標変換行列
struct TransformationMatrix
{
    float4x4 world;
    float4x4 worldViewProjection;
    float4x4 worldInverseTranspose;
};
StructuredBuffer<TransformationMatrix> gTransformation : register(t0);

// マテリアル
struct Material
{
	// 色
    float4 color;
	
	// uv行列
    float4x4 uvMatrix;

    // テクスチャインデックス
    uint textureIndex;
};
StructuredBuffer<Material> gMaterial : register(t1);

VertexShaderOutput main(VertexShaderInput input , uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    
    // 同次クリップ空間に変換
    output.position = mul(input.position, gTransformation[instanceID].worldViewProjection);
    
    // UV座標を渡す
    output.texcoord = input.texcoord;
    
    // ワールド空間での位置を計算
    output.worldPosition = mul(input.position, gTransformation[instanceID].world).xyz;
    
    // 法線をワールド空間に変換し、正規化する
    output.normal = normalize(mul(float4(input.normal, 0.0f), gTransformation[instanceID].worldInverseTranspose).xyz);

    // マテリアル情報を渡す
    output.color = gMaterial[instanceID].color;
    output.uvTransform = gMaterial[instanceID].uvMatrix;
    output.textureIndex = gMaterial[instanceID].textureIndex;
    
    return output;
}
