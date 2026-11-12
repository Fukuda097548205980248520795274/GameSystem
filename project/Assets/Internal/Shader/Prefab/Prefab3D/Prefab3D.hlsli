
struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION0;

    float4 color : COLOR0;
    float4x4 uvTransform : UVTRANSFORM0;
    uint textureIndex : TEXCOORD1;
};
