#pragma once
#include <functional>
#include <string>
#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptstdstring/scriptstdstring.h>
#include "Input/Input.hpp"
#include "Audio/AudioStore/AudioData.hpp"
#include "../EntityECS/EntityECS.hpp"
#include "Store/ModelStore/ModelData.hpp"
#include "Vector/Vector3/Vector3.hpp"
#include "Vector/Vector2/Vector2.hpp"
#include "Vector/Vector4/Vector4.hpp"
#include "Store/ProceduralMeshStore/ProceduralMeshStore.hpp"
#include "Script/ScriptManager/ScriptManager.hpp"
#include "Store/SkeletonStore/SkeletonData.hpp"
#include "Store/AnimationStore/AnimationData.hpp"
#include "Store/SkinningInstanceStore/SkinningInstance/SkinningInstance.hpp"
#include "Store/SkinClusterStore/SkinCluster/SkinCluster.hpp"

/// @brief スクリプトコンポーネント
struct ScriptComponent
{
	/// @brief コンストラクタ
	ScriptComponent() = default;

	/// @brief コンストラクタ
	/// @param handle 
	ScriptComponent(Detail::ScriptHandle handle) : handle(handle), filePath("") {}

	/// @brief デストラクタ
	~ScriptComponent()
	{
		if (scriptObject)
		{
			scriptObject->Release();
			scriptObject = nullptr;
		}
	}

	// @brief スクリプトのファイルパス
	std::string filePath;

	// スクリプトのモジュール名
	std::string moduleName;

	// スクリプトのクラス名
	std::string className;


	// @brief スクリプトハンドル
	Detail::ScriptHandle handle;


	// スクリプトオブジェクトのインスタンスを保持するためのメンバ変数
	asIScriptObject* scriptObject = nullptr;

	// スクリプトの更新関数（Update）を保持するためのメンバ変数
	asIScriptFunction* updateFunc = nullptr;

	// スクリプトの初期化関数（コンストラクタ）を保持するためのメンバ変数
	asIScriptFunction* initFunc = nullptr;
};

/// @brief ScriptComponent用の特殊化
template<>
inline void ComponentArray<ScriptComponent>::PostCopyProcess()
{
	// バックアップ側は再生時のインスタンス情報を保持する必要がないため、単純にポインタ参照をクリアする
	for (auto& scriptComp : dense_)
	{
		scriptComp.scriptObject = nullptr;
		scriptComp.initFunc = nullptr;
		scriptComp.updateFunc = nullptr;
	}
}


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

/// @brief 親子関係コンポーネント
struct HierarchyComponent
{
	HierarchyComponent() : parent(kNullEntity) {}

	/// @brief 親エンティティ
	Entity parent;

	/// @brief 子エンティティのリスト
	std::vector<Entity> children;
};

/// @brief ジョイント用親子関係コンポーネント
struct JointHierarchyComponent
{
	JointHierarchyComponent() : parent(kNullEntity) {}

	/// @brief 親エンティティ
	Entity parent;

	/// @brief 子エンティティのリスト
	std::vector<Entity> children;
};

// @brief ノードコンポーネント（モデルのノード情報を保持する）
struct NodeComponent
{
	// @brief ノードの名前
	std::string name;

	// @brief モデル内のノードインデックス
	int32_t index;
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

/// @brief プレハブコンポーネント
struct PrefabComponent
{
    PrefabComponent() : priority(0) {}

    // 描画順序（数値が小さいほど先に描画）
    int priority = 0;
};

// @brief プレハブのインスタンスを示すコンポーネント
struct PrefabInstanceComponent
{
    PrefabInstanceComponent() : sourcePrefab(kNullEntity) {}

    // 参照するPrefabのエンティティ
    Entity sourcePrefab;
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
	ModelComponent() : handle(Detail::ModelHandle::Invalid()) {}
	
	// 世代付きモデルハンドル
	Detail::ModelHandle handle;
};

/// @brief メッシュ描画コンポーネント
struct MeshDrawComponent
{
	MeshDrawComponent(int32_t meshIndex) : handle(Detail::ModelHandle::Invalid()), meshIndex(meshIndex) {}

	// 世代付きモデルハンドル
	Detail::ModelHandle handle;

	// メッシュのインデックス（モデル内のメッシュを指定するために使用される）
	int32_t meshIndex;

	// ルートエンティティの参照
	Entity rootMeshEntity;
};

/// @brief スケルトンコンポーネント
struct SkeletonComponent
{
	SkeletonComponent() : handle(Detail::SkeletonHandle::Invalid()){}

	// スケルトンハンドル
	Detail::SkeletonHandle handle;

	// キャラクター固有の姿勢
	Detail::SkeletonPose pose{};
};

/// @brief アニメーションコンポーネント
struct AnimationComponent
{
	AnimationComponent() : handle(Detail::AnimationHandle::Invalid()), currentTime(0.0f), playbackSpeed(1.0f), isPlaying(true), isLoop(true) {}

	// 再生するアニメーション
	Detail::AnimationHandle handle;

	// 再生時間
	float currentTime;

	// 再生速度
	float playbackSpeed;

	// 再生中かどうか
	bool isPlaying;

	// ループ再生するか
	bool isLoop;
};

/// @brief スキニングクラスターコンポーネント（各エンティティ固有の動的データ）
struct SkinClusterComponent
{
	SkinClusterComponent() : handle(Detail::SkinClusterHandle::Invalid()) {}

	// 動的なボーン行列パレットやスキニング後頂点バッファを保持するハンドル
	Detail::SkinClusterHandle handle;
};

// @brief スキニングインスタンスコンポーネント（各エンティティ固有の動的データ）
struct SkinningInstanceComponent
{
	SkinningInstanceComponent() : handle(Detail::SkinningInstanceHandle::Invalid()) {}

	// スキニングインスタンスハンドル
	Detail::SkinningInstanceHandle handle;
};

// @brief カメラの投影モード
enum class ProjectionMode
{
    // 平行投影 (2D用)
    Orthographic,

    // 透視投影 (3D用)
    Perspective
};

/// @brief カメラコンポーネント
struct CameraComponent
{
    CameraComponent() : projectionMode(ProjectionMode::Perspective),
        orthographicSize(1.0f), fovY(0.35f), aspectRatio(16.0f / 9.0f), nearPlane(0.03f), farPlane(800.0f) {
    }


    // @brief 投影モード（Perspective: 透視投影, Orthographic: 平行投影）
    ProjectionMode projectionMode;


	/// @brief 垂直方向の視野角（度）
	float fovY;

	/// @brief アスペクト比（幅 / 高さ）
	float aspectRatio;

	/// @brief ニアクリップ平面
	float nearPlane;

	/// @brief ファークリップ平面
	float farPlane;

    // 平行投影のサイズ（高さ）
    float orthographicSize;
};

// @brief デバッグカメラコンポーネント
struct DebugCameraComponent
{
    DebugCameraComponent() : moveSpeed(10.0f), lookSensitivity(0.005f) {}

    // @brief カメラの移動速度
    float moveSpeed;

    // @brief マウスの感度（回転速度）
    float lookSensitivity;
};

// コンスタントバッファコンポーネント
struct ConstantBufferComponent
{
    ConstantBufferComponent(uint32_t handle) : handle(handle) {}

    // @brief 定数バッファのハンドル
    uint32_t handle;
};

/// @brief トランスフォーム用定数バッファコンポーネント
struct TransformConstantBufferComponent : public ConstantBufferComponent { TransformConstantBufferComponent(uint32_t handle) : ConstantBufferComponent(handle) {} };

/// @brief マテリアル用定数バッファコンポーネント
struct MaterialConstantBufferComponent : public ConstantBufferComponent { MaterialConstantBufferComponent(uint32_t handle) : ConstantBufferComponent(handle) {} };

/// @brief スカイボックスコンポーネント
struct SkyboxComponent
{
	SkyboxComponent(){}

	/// @brief スカイボックスの有効/無効フラグ
	bool isEnabled = true;
};

// @brief 影をキャストするライト用のコンポーネント
struct ShadowCasterComponent
{
    ShadowCasterComponent() : lightViewProjectionMatrix(MakeIdentityMatrix4x4()), isEnabled(true) {}

    // ライト視点のビュー射影行列
    Matrix4x4 lightViewProjectionMatrix;

    // シャドウの有効/無効フラグ
    bool isEnabled = true;

    // シャドウマップのテクスチャハンド
    uint32_t shadowMapTextureHandle = 0;
};

// @brief 影を出すかどうかのフラグを持つコンポーネント
struct CastShadowComponent
{
    // シャドウをキャストするかどうかのフラグ
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
	/// @brief コンストラクタ
	TextureComponent();

	/// @brief コンストラクタ
	/// @param handle 
	TextureComponent(Detail::TextureHandle handle) : handle(handle), filePath("") {}

	// @brief テクスチャのファイルパス
	std::string filePath;

	/// @brief テクスチャのハンドル（またはID/ポインタ）
	Detail::TextureHandle handle;
};

/// @brief キューブマップコンポーネント
struct CubemapComponent
{
	/// @brief コンストラクタ
	CubemapComponent();

	/// @brief コンストラクタ
	/// @param handle 
	CubemapComponent(Detail::TextureHandle handle) : handle(handle), filePath("") {}

	// @brief キューブマップのファイルパス
	std::string filePath;

	/// @brief キューブマップのハンドル（またはID/ポインタ）
	Detail::TextureHandle handle;
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
    DirectionalLightComponent() : position(0.0f, 0.0f, 0.0f), direction(0.0f, -1.0f, 0.0f), color(1.0f, 1.0f, 1.0f), intensity(1.0f) {}

	/// @brief 有効 / 無効フラグ
	bool isEnabled = true;

    /// @brief 位置
    Vector3 position;

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

	/// @brief 有効 / 無効フラグ
	bool isEnabled = true;

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

	/// @brief 有効 / 無効フラグ
	bool isEnabled = true;

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


/// @brief オーディオコンポーネント
struct AudioComponent
{
	AudioComponent() :
		filePath(""),
		audioHandle(Detail::AudioHandle::Invalid()),
		volume(1.0f),
		pitch(1.0f),
		isLoop(false),
		allowOverlap(false){
	}

	// @brief 再生するオーディオファイルのパス
	std::string filePath;

	/// @brief 再生するオーディオデータのハンドル（AudioStore::Loadの戻り値）
	Detail::AudioHandle audioHandle;

	// 複数のハンドルを管理するリスト
	std::vector<Detail::PlayHandle> playHandles;

	/// @brief 音量 (0.0f ～ 1.0f)
	float volume;

	/// @brief ピッチ周波数比
	float pitch;

	/// @brief ループ再生するか
	bool isLoop;

	// 重複再生（多重再生）を許可するか
	bool allowOverlap;
};



/// @brief 入力デバイスの種類
enum class InputDeviceType
{
	KeyboardMouse,
	Gamepad
};

/// @brief 入力コンポーネント
struct InputComponent
{
	InputComponent() :
		deviceType(InputDeviceType::KeyboardMouse),
		gamepadNumber(0),
		moveUpKey(DIK_W),      // DIK_W
		moveDownKey(DIK_S),    // DIK_S
		moveLeftKey(DIK_A),    // DIK_A
		moveRightKey(DIK_D),   // DIK_D
		actionKey(DIK_SPACE),      // DIK_SPACE
		actionButton(XINPUT_GAMEPAD_A), // XINPUT_GAMEPAD_A
		moveInput(0.0f, 0.0f),
		cameraInput(0.0f, 0.0f),
		isActionPressed(false),
		isActionTriggered(false) {
	}

	/// @brief 使用する入力デバイス
	InputDeviceType deviceType;

	/// @brief ゲームパッド番号（0～3）
	int gamepadNumber;

	// --- キーボード割り当て（DirectInputの DIK_* を想定） ---
	uint32_t moveUpKey;
	uint32_t moveDownKey;
	uint32_t moveLeftKey;
	uint32_t moveRightKey;
	uint32_t actionKey;

	// --- ゲームパッド割り当て（XInputの XINPUT_GAMEPAD_* を想定） ---
	uint32_t actionButton;

	// --- 現在の入力状態（InputSystem側で毎フレーム更新される値） ---
	Vector2 moveInput;
	Vector2 cameraInput;
	bool isActionPressed;
	bool isActionTriggered;
};
