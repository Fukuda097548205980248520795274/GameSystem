
// カメラ情報
struct Camera
{
	// カメラの近平面距離
    float nearZ;
	
	// カメラの遠平面距離
    float farZ;
	
	 // スクリーンの幅と高さ（ピクセル単位）
    float2 screenDimensions;
	
	// カメラのワールド空間での位置
    float3 worldPosition;
	
	// ビュー行列
    float4x4 view;
	
	// 投影行列
    float4x4 projection;
	
	// 逆投影行列
    float4x4 inverseProjection;
};