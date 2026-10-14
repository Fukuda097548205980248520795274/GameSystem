#pragma once
#include <functional>
#include <string>
#include "Vector/Vector3/Vector3.h"
#include "Vector/Vector2/Vector2.h"
#include "Vector/Vector4/Vector4.h"
#include "Store/ProceduralMeshStore/ProceduralMeshStore.h"

/// @brief トランスフォームコンポーネント
struct TransformComponent
{
	/// @brief コンストラクタ
	TransformComponent() : position(0.0f, 0.0f, 0.0f), rotation(0.0f, 0.0f, 0.0f), scale(1.0f, 1.0f, 1.0f) {}

	/// @brief 位置
	Vector3 position;

	/// @brief 回転
	Vector3 rotation;

	/// @brief スケール
	Vector3 scale;
};

/// @brief ブレンドモードの列挙型
enum class BlendMode
{
	None,
	Normal,
	Add,
	Subtract,
	Multiply,
	Screen,
};

/// @brief パスの基本的な制御と実行順序
struct RenderPassComponent
{
	RenderPassComponent() : isEnabled(true), priority(0) {}

	/// @brief パスの有効/無効フラグ
	bool isEnabled;

	/// @brief 描画順序（数値が小さいほど先に描画）
	int priority;
};

/// @brief 描画コンポーネント
struct RenderComponent
{
	RenderComponent() : isEnabled(true), priority(0) {}

	/// @brief 3D描画の有効/無効フラグ
	bool isEnabled;

	// 描画順序（数値が小さいほど先に描画）
	int priority;
};

/// @brief メッシュコンポーネント
struct MeshComponent
{
	MeshComponent() : meshType(Detail::ProceduralMeshType::Cube) {}

	/// @brief メッシュの種類
	Detail::ProceduralMeshType meshType;
};

/// @brief モデルコンポーネント
struct ModelComponent
{
	ModelComponent() : modelHandle(0) {}
	
	// モデルのハンドル（またはID/ポインタ）
	uint32_t modelHandle;
};

/// @brief カメラコンポーネント
struct CameraComponent
{
	CameraComponent() : fovY(0.35f), aspectRatio(16.0f / 9.0f), nearPlane(0.01f), farPlane(800.0f) {}

	/// @brief 垂直方向の視野角（度）
	float fovY;

	/// @brief アスペクト比（幅 / 高さ）
	float aspectRatio;

	/// @brief ニアクリップ平面
	float nearPlane;

	/// @brief ファークリップ平面
	float farPlane;
};

/// @brief 定数バッファコンポーネント
struct ConstantBufferComponent
{
	ConstantBufferComponent(uint32_t handle) : handle(handle) {}

	/// @brief 定数バッファのハンドル（またはID/ポインタ）
	uint32_t handle;
};

/// @brief トランスフォーム用定数バッファコンポーネント
struct TransformConstantBufferComponent : public ConstantBufferComponent
{
	TransformConstantBufferComponent(uint32_t handle) : ConstantBufferComponent(handle) {}
};

/// @brief マテリアル用定数バッファコンポーネント
struct MaterialConstantBufferComponent : public ConstantBufferComponent
{
	MaterialConstantBufferComponent(uint32_t handle) : ConstantBufferComponent(handle) {}
};

/// @brief スカイボックスコンポーネント
struct SkyboxComponent
{
	SkyboxComponent(){}

	/// @brief スカイボックスの有効/無効フラグ
	bool isEnabled = true;
};

/// @brief 合成（ブレンド）のパラメータ
struct BlendComponent
{
	BlendComponent() : opacity(1.0f), blendMode(BlendMode::Normal) {}

	/// @brief 不透明度（0.0f ～ 1.0f）
	float opacity;

	/// @brief ブレンドモード
	BlendMode blendMode;
};

/// @brief 入力テクスチャと出力先レンダーターゲット
struct RenderTargetComponent
{
	RenderTargetComponent() : inputTextureHandle(0), outputTargetHandle(0) {}

	/// @brief 読み込むテクスチャのハンドル（またはID/ポインタ）
	uint32_t inputTextureHandle;

	/// @brief 書き込むレンダーターゲットのハンドル（またはID/ポインタ）
	uint32_t outputTargetHandle;
};

/// @brief テクスチャコンポーネント
struct TextureComponent
{
	TextureComponent(uint32_t handle) : handle(handle) {}

	/// @brief テクスチャのハンドル（またはID/ポインタ）
	uint32_t handle;
};

/// @brief キューブマップコンポーネント
struct CubemapComponent
{
	CubemapComponent(uint32_t handle) : handle(handle) {}

	/// @brief キューブマップのハンドル（またはID/ポインタ）
	uint32_t handle;
};

/// @brief マテリアルコンポーネント
struct MaterialComponent
{
	MaterialComponent() : psoName("Default_Render3D"), color(1.0f, 1.0f, 1.0f, 1.0f) {}

	/// @brief 使用するPSO名（PSOEditorで管理されている名前）
	std::string psoName;

	/// @brief マテリアルの色
	Vector4 color;
};

/// @brief UVトランスフォームコンポーネント
struct UVTransformComponent
{
	UVTransformComponent() : offset(0.0f, 0.0f), scale(1.0f, 1.0f), radian(0.0f) {}

	/// @brief UVオフセット
	Vector2 offset;

	/// @brief UVスケール
	Vector2 scale;

	/// @brief 回転角度（ラジアン）
	float radian;
};

/// @brief ディレクショナルライトコンポーネント
struct DirectionalLightComponent
{
	DirectionalLightComponent() : direction(0.0f, -1.0f, 0.0f), color(1.0f, 1.0f, 1.0f), intensity(1.0f) {}

	/// @brief 光の方向（正規化されたベクトル）
	Vector3 direction;

	/// @brief 光の色
	Vector3 color;

	/// @brief 光の強さ
	float intensity;
};

/// @brief ポイントライトコンポーネント
struct PointLightComponent
{
	PointLightComponent() : position(0.0f, 0.0f, 0.0f), color(1.0f, 1.0f, 1.0f), intensity(1.0f), radius(1.0f), decay(1.0f) {}

	/// @brief 光の位置
	Vector3 position;

	/// @brief 光の色
	Vector3 color;

	/// @brief 光の強さ
	float intensity;

	// @brief 半径
	float radius;

	// @brief 減衰率
	float decay;
};

/// @brief スポットライトコンポーネント
struct SpotLightComponent
{
	SpotLightComponent() : position(0.0f, 0.0f, 0.0f), direction(0.0f, -1.0f, 0.0f), color(1.0f, 1.0f, 1.0f),
		intensity(1.0f), distance(1.0f), decay(1.0f), cosAngle(1.0f), cosFalloffStart(1.0f) {}

	/// @brief 光の位置
	Vector3 position;

	/// @brief 光の方向（正規化されたベクトル）
	Vector3 direction;

	/// @brief 光の色
	Vector3 color;

	/// @brief 光の強さ
	float intensity;

	// @brief 半径
	float distance;

	// @brief 減衰率
	float decay;

	// @brief スポットライトの角度（コサイン値）
	float cosAngle;

	// @brief スポットライトの減衰開始角度（コサイン値）
	float cosFalloffStart;
};