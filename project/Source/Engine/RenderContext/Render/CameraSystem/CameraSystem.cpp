#include "CameraSystem.h"
#include "Engine.h"

/// @brief 初期化
void Detail::CameraSystem::Initialize()
{
	// Engineのインスタンスを取得する
	auto  engine = Engine::GetInstance();
	assert(engine);

	// ECSレジストリを取得する
	auto registry = engine->GetRegistryECS();
	assert(registry);

	// メンバ変数に代入する
	registry_ = registry;
}

/// @brief 更新処理
void Detail::CameraSystem::Update()
{
	// 現在のカメラが設定されていない場合は何もしない
	if (currentEntity_ == kNullEntity)
	{
		cameraMatrix_ = MakeIdentityMatrix4x4();
		viewMatrix_ = MakeIdentityMatrix4x4();
		projectionMatrix_ = MakeIdentityMatrix4x4();
		return;
	}

	// TransformComponentを取得する
	auto transform = registry_->GetComponent<TransformComponent>(currentEntity_);
	if (!transform) return;
	
	// 回転をクォータニオンに変換する
	Quaternion rotationQuat = 
		ToQuaternion(transform->rotation.z, Vector3(0.0f, 0.0f, 1.0f)) *
		ToQuaternion(transform->rotation.y, Vector3(0.0f, 1.0f, 0.0f)) *
		ToQuaternion(transform->rotation.x, Vector3(1.0f, 0.0f, 0.0f));

	// カメラ行列
	cameraMatrix_ = Make3DAffineMatrix4x4(transform->scale, rotationQuat, transform->position);

	// ビュー行列はカメラ行列の逆行列
	viewMatrix_ = cameraMatrix_.Inverse();

	// CameraComponentを取得する
	auto cameraComp = registry_->GetComponent<CameraComponent>(currentEntity_);
	if (!cameraComp) return;

	// 射影行列を計算する
	projectionMatrix_ = MakePerspectiveFovMatrix4x4(cameraComp->fovY, cameraComp->aspectRatio, cameraComp->nearPlane, cameraComp->farPlane);

	// 近平面と遠平面の距離を保存する
	nearZ_ = cameraComp->nearPlane;
	farZ_ = cameraComp->farPlane;
}

/// @brief エンティティを作成する
/// @return 
Entity Detail::CameraSystem::CreateCameraEntity()
{
	// エンティティを作成する
	Entity entity = registry_->CreateEntity();

	// TransformComponentを追加する
	TransformComponent transformComp;
	registry_->AddComponent(entity, transformComp);

	// CameraComponentを追加する
	CameraComponent cameraComp;
	registry_->AddComponent(entity, cameraComp);

	// もし現在のカメラが設定されていない場合は、このカメラを現在のカメラとして設定する
	if (currentEntity_ == kNullEntity)
		currentEntity_ = entity;

	return entity;
}