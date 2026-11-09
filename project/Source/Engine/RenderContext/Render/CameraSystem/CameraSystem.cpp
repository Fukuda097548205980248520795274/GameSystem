#include "CameraSystem.hpp"
#include "Engine.hpp"

/// @brief 初期化
void Detail::CameraSystem::Initialize()
{
	// Engineのインスタンスを取得する
	auto  engine = Engine::GetInstance();
	assert(engine);

	// ECSレジストリを取得する
	auto registry = engine->GetRegistryECS();
	assert(registry);
}

/// @brief 更新処理
void Detail::CameraSystem::Update()
{
	// ECSレジストリを取得する
	auto registry = Engine::GetInstance()->GetRegistryECS();
	if (!registry) return;

	// 現在のカメラが設定されていない場合は何もしない
	if (currentEntity_ == kNullEntity)
	{
		cameraMatrix_ = MakeIdentityMatrix4x4();
		viewMatrix_ = MakeIdentityMatrix4x4();
		projectionMatrix_ = MakeIdentityMatrix4x4();
		return;
	}

	// TransformComponentを取得する
	auto transform = registry->GetComponent<TransformComponent>(currentEntity_);
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
	auto cameraComp = registry->GetComponent<CameraComponent>(currentEntity_);
	if (!cameraComp) return;

    if (cameraComp->projectionMode == ProjectionMode::Orthographic)
    {
        // 平行投影の場合は、幅と高さを使用して射影行列を計算する
        projectionMatrix_ = MakeOrthographicMatrix4x4(-cameraComp->width, cameraComp->height, cameraComp->width, -cameraComp->height, 0.0f, cameraComp->farPlane);
    }
    else
    {
        // 透視投影の場合は、FOVとアスペクト比を使用して射影行列を計算する
        projectionMatrix_ = MakePerspectiveFovMatrix4x4(cameraComp->fovY, cameraComp->aspectRatio, cameraComp->nearPlane, cameraComp->farPlane);
    }

	// 近平面と遠平面の距離を保存する
	nearZ_ = cameraComp->nearPlane;
	farZ_ = cameraComp->farPlane;
}

/// @brief エンティティを作成する
/// @return 
Entity Detail::CameraSystem::CreateCameraEntity()
{
	// ECSレジストリを取得する
	auto registry = Engine::GetInstance()->GetRegistryECS();
	if (!registry) return kNullEntity;


    Entity entity = registry->CreateEntity();
    registry->AddComponent(entity, TransformComponent());
    registry->AddComponent(entity, CameraComponent());


	// もし現在のカメラが設定されていない場合は、このカメラを現在のカメラとして設定する
	if (currentEntity_ == kNullEntity)
		currentEntity_ = entity;

	return entity;
}
