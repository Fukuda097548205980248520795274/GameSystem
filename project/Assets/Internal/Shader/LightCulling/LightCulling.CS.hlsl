#include "LightData.hlsli"
#include "../Camera/CameraData.hlsli"

// カメラ情報
ConstantBuffer<Camera> gCamera : register(b0);

// ライトの数
ConstantBuffer<NumLight> gNumLight : register(b1);

// ライト情報のバッファ
StructuredBuffer<PointLight> gPointLight : register(t0);
StructuredBuffer<SpotLight> gSpotLight : register(t1);

// Zバッファ
Texture2D<float> gDepthTexture : register(t2);

// タイルごとのライトインデックスを格納するバッファ
RWStructuredBuffer<uint> gTileLightIndexBuffer : register(u0);

// タイルサイズ
groupshared uint s_MinDepthInt;
groupshared uint s_MaxDepthInt;

// タイル内のライトの数を格納する共有メモリ
groupshared uint s_PointLightCount;
groupshared uint s_SpotLightCount;

// タイル内のライトインデックスを格納する共有メモリ
groupshared uint s_PointLightIndices[MAX_LIGHTS_PER_TILE];
groupshared uint s_SpotLightIndices[MAX_LIGHTS_PER_TILE];

// 視錐台の平面を格納する共有メモリ
groupshared float4 s_FrustumPlanes[6];

// floatをuintに変換する関数
uint FloatToAsUint(float f)
{
	return asuint(f);
}

// uintをfloatに変換する関数
float AsUintToFloat(uint i)
{
	return asfloat(i);
}

// NDC座標からビュー空間への変換関数
float3 ScreenToView(float2 screenPos, float depth, float2 screenDimensions, float4x4 inverseProjection)
{
	// スクリーンのピクセル座標をNDC空間 (-1.0 ～ 1.0) に変換
	float2 ndc;
	ndc.x = (screenPos.x / screenDimensions.x) * 2.0f - 1.0f;
	ndc.y = -(screenPos.y / screenDimensions.y) * 2.0f + 1.0f;

	// NDC座標を逆投影行列で変換
	float4 clipPos = float4(ndc, depth, 1.0f);
	float4 viewPos = mul(clipPos, inverseProjection);
	
	// 同次座標のW除算を行って3D座標を取得
	return viewPos.xyz / viewPos.w;
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 groupID : SV_GroupID,uint3 groupThreadID : SV_GroupThreadID,uint3 dispatchThreadID : SV_DispatchThreadID)
{
	// 共有メモリの初期化（グループ内の先頭スレッドのみ）
	uint localIndex = groupThreadID.y * TILE_SIZE + groupThreadID.x;
	if (localIndex == 0)
	{
		s_MinDepthInt = 0x7F7FFFFF; // floatの最大値
		s_MaxDepthInt = 0;
		s_PointLightCount = 0;
		s_SpotLightCount = 0;
	}
	
	// 共有メモリの初期化を全スレッドで使用できるように同期
	GroupMemoryBarrierWithGroupSync();

	
	// タイル内の各ピクセルの深度を取得して、共有メモリに最小/最大深度を格納
	if (dispatchThreadID.x < (uint) gCamera.screenDimensions.x && dispatchThreadID.y < (uint) gCamera.screenDimensions.y)
	{
		float depth = gDepthTexture.Load(int3(dispatchThreadID.x, dispatchThreadID.y, 0));
		InterlockedMin(s_MinDepthInt, FloatToAsUint(depth));
		InterlockedMax(s_MaxDepthInt, FloatToAsUint(depth));
	}
	
	// 共有メモリの最小/最大深度を全スレッドで使用できるように同期
	GroupMemoryBarrierWithGroupSync();

	
	// タイルの視錐台を計算（グループ内の先頭スレッドのみ）
	if (localIndex == 0)
	{
		// 共有メモリからMin/Max深度を取得
        float minDepth = 0.0f;
		float maxDepth = 1.0f;

		// タイルのスクリーン座標範囲を計算
        float2 tileMin = float2(groupID.x * TILE_SIZE, groupID.y * TILE_SIZE);
        float2 tileMax = float2(min((groupID.x + 1) * TILE_SIZE, gCamera.screenDimensions.x), min((groupID.y + 1) * TILE_SIZE, gCamera.screenDimensions.y));

		// タイルの4つの角のビュー空間座標を計算
		float3 viewTopLeft = ScreenToView(float2(tileMin.x, tileMin.y), 1.0f, gCamera.screenDimensions, gCamera.inverseProjection);
		float3 viewBottomLeft = ScreenToView(float2(tileMin.x, tileMax.y), 1.0f, gCamera.screenDimensions, gCamera.inverseProjection);
		float3 viewTopRight = ScreenToView(float2(tileMax.x, tileMin.y), 1.0f, gCamera.screenDimensions, gCamera.inverseProjection);
		float3 viewBottomRight = ScreenToView(float2(tileMax.x, tileMax.y), 1.0f, gCamera.screenDimensions, gCamera.inverseProjection);

		// 視錐台の6つの平面を計算（法線ベクトルと距離を格納）
		s_FrustumPlanes[0] = float4(normalize(cross(viewTopLeft, viewBottomLeft)), 0.0f); 
		s_FrustumPlanes[1] = float4(normalize(cross(viewBottomRight, viewTopRight)), 0.0f);
		s_FrustumPlanes[2] = float4(normalize(cross(viewTopRight, viewTopLeft)), 0.0f);
		s_FrustumPlanes[3] = float4(normalize(cross(viewBottomLeft, viewBottomRight)), 0.0f); 

		// 近クリップ平面と遠クリップ平面の距離を計算
        float3 viewMin = ScreenToView(float2(0.0f, 0.0f), minDepth, gCamera.screenDimensions, gCamera.inverseProjection);
        float3 viewMax = ScreenToView(float2(0.0f, 0.0f), maxDepth, gCamera.screenDimensions, gCamera.inverseProjection);

		// 近クリップ平面と遠クリップ平面の法線はZ軸方向
		s_FrustumPlanes[4] = float4(0.0f, 0.0f, 1.0f, -viewMin.z); 
		s_FrustumPlanes[5] = float4(0.0f, 0.0f, -1.0f, viewMax.z);
	}
	
	// 共有メモリの視錐台平面を全スレッドで使用できるように同期
	GroupMemoryBarrierWithGroupSync();

	
	// ポイントライトの交差判定（各スレッドが分担して処理）
	for (uint i = localIndex; i < (uint) gNumLight.pointLight; i += TILE_SIZE * TILE_SIZE)
	{
		PointLight light = gPointLight[i];
		
		// ライト位置をビュー空間に変換
        float3 viewLightPos = mul(float4(light.position, 1.0f), gCamera.view).xyz;
		
		bool inFrustum = true;
		
		// ポイントライトは球体として判定する
		for (int p = 0; p < 6; ++p)
		{
            float distance = dot(s_FrustumPlanes[p].xyz, viewLightPos) + s_FrustumPlanes[p].w;
			
			// 平面の外側（マイナス方向）に半径以上離れていれば、完全に範囲外
			if (distance < -light.radius)
			{
				inFrustum = false;
				break;
			}
		}
		
		// 範囲内のライトを共有メモリに追加
		if (inFrustum)
		{
			uint index;
			InterlockedAdd(s_PointLightCount, 1, index);
			
			// インデックスが最大数を超えないように制限
			if (index < MAX_LIGHTS_PER_TILE)
			{
				s_PointLightIndices[index] = i;
			}
		}
	}

	// スポットライトの交差判定（同様に手分けして処理）
	for (uint j = localIndex; j < (uint) gNumLight.spotLight; j += TILE_SIZE * TILE_SIZE)
	{
		SpotLight light = gSpotLight[j];
		
		// ライト位置をビュー空間に変換
		float3 viewLightPos = mul(float4(light.position, 1.0f), gCamera.view).xyz;
		
		bool inFrustum = true;
		
		// スポットライトは簡易的に distance を半径とした球体として判定する
		for (int p = 0; p < 6; ++p)
		{
			float d = dot(s_FrustumPlanes[p].xyz, viewLightPos) + s_FrustumPlanes[p].w;
			
			// 平面の外側（マイナス方向）に distance 以上離れていれば、完全に範囲外
			if (d < -light.distance)
			{
				inFrustum = false;
				break;
			}
		}
		
		// 範囲内のライトを共有メモリに追加
		if (inFrustum)
		{
			uint index;
			InterlockedAdd(s_SpotLightCount, 1, index);
			
			// インデックスが最大数を超えないように制限
			if (index < MAX_LIGHTS_PER_TILE)
			{
				s_SpotLightIndices[index] = j;
			}
		}
	}
	
	// 共有メモリのライトインデックスを全スレッドで使用できるように同期
	GroupMemoryBarrierWithGroupSync();

	
	// タイルごとのライトインデックスをグローバルバッファに書き込む
    uint numTilesX = (uint) ceil(gCamera.screenDimensions.x / (float) TILE_SIZE);

	// タイルのインデックスを計算
    uint tileIndex = groupID.y * numTilesX + groupID.x;

	// バッファのオフセットを計算
    uint bufferOffset = tileIndex * (MAX_LIGHTS_PER_TILE * 2 + 2);

	// ポイントライトとスポットライトの個数を制限して取得
    uint ptCount = min(s_PointLightCount, MAX_LIGHTS_PER_TILE);
    uint spCount = min(s_SpotLightCount, MAX_LIGHTS_PER_TILE);

	// スポットライトのインデックスの開始位置を計算
    uint spOffset = bufferOffset + MAX_LIGHTS_PER_TILE + 1;

	// 個数はスレッド0だけが書き込む
    if (localIndex == 0)
    {
        gTileLightIndexBuffer[bufferOffset] = ptCount;
        gTileLightIndexBuffer[spOffset] = spCount;
    }

	// インデックスは全スレッドで分担して書き込む
    for (uint p = localIndex; p < ptCount; p += TILE_SIZE * TILE_SIZE)
    {
        gTileLightIndexBuffer[bufferOffset + 1 + p] = s_PointLightIndices[p];
    }

	// スポットライトのインデックスも全スレッドで分担して書き込む
    for (uint s = localIndex; s < spCount; s += TILE_SIZE * TILE_SIZE)
    {
        gTileLightIndexBuffer[spOffset + 1 + s] = s_SpotLightIndices[s];
    }
}