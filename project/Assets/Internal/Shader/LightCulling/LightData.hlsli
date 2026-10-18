
// タイルベースのライトカリング用コンピュートシェーダー
#define TILE_SIZE 16

// タイルごとの最大ライト数（ポイントライト + スポットライト）
#define MAX_LIGHTS_PER_TILE 256

// 平行光源
struct DirectionalLight
{
	// 色（RGBA）
    float4 color;
	
	// ライトの方向（正規化済み）
    float3 direction;
	
	// ライトの強度（明るさ）
    float intensity;
};

// ポイントライト
struct PointLight
{
	// 色（RGBA）
    float4 color;
	
	// 位置
    float3 position;
	
	// ライトの強度（明るさ）
    float intensity;
	
	// ライトの影響範囲（半径）
    float radius;
	
	// 減衰率
    float decay;
};

// スポットライト
struct SpotLight
{
	// 色（RGBA）
    float4 color;
	
	// 位置
    float3 position;
	
	// ライトの強度（明るさ）
    float intensity;
	
	// スポットライトの方向（正規化済み）
    float3 direction;
	
	// ライトの影響範囲（半径）
    float distance;
	
	// 減衰率
    float decay;
	
	// スポットライトの角度（ラジアン）
    float cosAngle;
	
	// スポットライトの減衰開始角度のコサイン値
    float cosFalloffStart;
};

// ライトの数
struct NumLight
{
	// 平行光源の数
    int directionalLight;
	
	// ポイントライトの数
    int pointLight;
	
	// スポットライトの数
    int spotLight;
};