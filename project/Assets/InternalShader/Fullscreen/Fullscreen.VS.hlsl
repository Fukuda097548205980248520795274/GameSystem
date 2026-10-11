#include "Fullscreen.hlsli"

// 頂点数
static const uint kNumVertex = 3;

// 初期位置
static const float4 kPosition[kNumVertex] =
{
    // 左上
    { -1.0f, 1.0f, 0.0f, 1.0f },
    
    // 右上
    { 3.0f, 1.0f, 0.0f, 1.0f },
    
    // 左下
    { -1.0f, -3.0f, 0.0f, 1.0f }
};

// 初期UV座標
static const float2 kTexcoord[kNumVertex] =
{
    // 左上
    { 0.0f, 0.0f },
    
    // 右上
    { 2.0f, 0.0f },
    
    // 左下
    { 0.0f, 2.0f }
};

VertexShaderOutput main(uint vertexID : SV_VertexID)
{
    VertexShaderOutput output;
    
    output.position = kPosition[vertexID];
    output.texcoord = kTexcoord[vertexID];
    
    return output;
}