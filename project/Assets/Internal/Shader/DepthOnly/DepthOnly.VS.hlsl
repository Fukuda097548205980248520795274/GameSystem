

// 入力頂点シェーダ
struct VertexShaderInput
{
    float4 position : POSITION0;
};

// 出力頂点シェーダ
struct VertexShaderOutput
{
    float4 position : SV_POSITION;
};

// 座標変換行列
struct TransformationMatrix
{
    float4x4 world;
    float4x4 worldViewProjection;
    float4x4 worldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformation : register(b0);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // 同次クリップ空間に変換
    output.position = mul(input.position, gTransformation.worldViewProjection);
    
    return output;
}