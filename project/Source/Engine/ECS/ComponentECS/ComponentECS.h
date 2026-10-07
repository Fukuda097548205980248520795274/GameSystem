#pragma once
#include <functional>
#include "Vector/Vector3/Vector3.h"
#include "Vector/Vector2/Vector2.h"
#include "Vector/Vector4/Vector4.h"

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
	TextureComponent() : handle(0) {}

	/// @brief テクスチャのハンドル（またはID/ポインタ）
	uint32_t handle;
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