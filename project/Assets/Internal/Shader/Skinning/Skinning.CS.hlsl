// ウェイトの影響力
struct Well
{
	float4x4 skeletonSpaceMatrix;
	float4x4 skeletonSpaceInverseTransposeMatrix;
};
StructuredBuffer<Well> gMatrixPalette : register(t0);

// 入力 : 頂点
struct Vertex
{
	float4 position;
	float2 texcoord;
	float3 normal;
};
StructuredBuffer<Vertex> gInputVertices : register(t1);

// 頂点とボーンの結びつき（影響度）
struct VertexInfluence
{
	float4 weight;
	int4 index;
};
StructuredBuffer<VertexInfluence> gInfluence : register(t2);

// 出力 : 頂点
RWStructuredBuffer<Vertex> gOutputVertices : register(u0);

// スキニング情報
struct SkinningInformation
{
	uint numVertices;
};
ConstantBuffer<SkinningInformation> gSkinningInformation : register(b0);



[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint vertexIndex = DTid.x;
	if (vertexIndex < gSkinningInformation.numVertices)
	{
		// 必要なデータを受け取る
		Vertex input = gInputVertices[vertexIndex];
		VertexInfluence influence = gInfluence[vertexIndex];
		
		// スキニング後の頂点を計算
		Vertex skinned;
		skinned.texcoord = input.texcoord;
		
		// 位置の変換を行う
		skinned.position = mul(input.position, gMatrixPalette[gInfluence[vertexIndex].index.x].skeletonSpaceMatrix) * gInfluence[vertexIndex].weight.x;
		skinned.position += mul(input.position, gMatrixPalette[gInfluence[vertexIndex].index.y].skeletonSpaceMatrix) * gInfluence[vertexIndex].weight.y;
		skinned.position += mul(input.position, gMatrixPalette[gInfluence[vertexIndex].index.z].skeletonSpaceMatrix) * gInfluence[vertexIndex].weight.z;
		skinned.position += mul(input.position, gMatrixPalette[gInfluence[vertexIndex].index.w].skeletonSpaceMatrix) * gInfluence[vertexIndex].weight.w;
	
		// 1.0fを入れる
		skinned.position.w = 1.0f;
		
		// 法線の変換を行う
		skinned.normal = mul(input.normal, (float3x3) gMatrixPalette[gInfluence[vertexIndex].index.x].skeletonSpaceInverseTransposeMatrix) * gInfluence[vertexIndex].weight.x;
		skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[gInfluence[vertexIndex].index.y].skeletonSpaceInverseTransposeMatrix) * gInfluence[vertexIndex].weight.y;
		skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[gInfluence[vertexIndex].index.z].skeletonSpaceInverseTransposeMatrix) * gInfluence[vertexIndex].weight.z;
		skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[gInfluence[vertexIndex].index.w].skeletonSpaceInverseTransposeMatrix) * gInfluence[vertexIndex].weight.w;
		skinned.normal = normalize(skinned.normal);
		
		// スキニング後の頂点データを格納
		gOutputVertices[vertexIndex] = skinned;
	}
}