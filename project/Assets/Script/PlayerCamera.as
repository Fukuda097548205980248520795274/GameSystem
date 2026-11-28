class PlayerCamera
{
	// プレイヤーエンティティ
	Entity playerEntity;


	// プレイヤーの足元からのオフセット
    Vector3 targetOffset = Vector3(0.0f, 1.5f, 0.0f);

	// 追従の滑らかさ（大きいほど即座に追従）
    float followSmoothness = 10.0f;


    // 制限値
    float minPhi = -0.4f;  // 見上げ限界（ラジアン）
    float maxPhi = 1.2f;   // 見下ろし限界（ラジアン）

	PlayerCamera()
	{
		// コンストラクタ
	}

	void Initialize()
	{
		// コンポーネントを取得
        TransformComponent playerTransform = GetTransformComponent(playerEntity);
        PivotPointComponent cameraPivot = GetPivotPointComponent();

		// カメラのピボットポイントをプレイヤーの位置に設定
        cameraPivot.pivotPoint = playerTransform.position + targetOffset;

		// ピボットポイントを設定
        SetPivotPointComponent(cameraPivot);
	}

	void Update()
	{
		// デルタタイムを取得
		float dt = GetDeltaTime();

        // コンポーネント取得
        TransformComponent cameraTransform = GetTransformComponent();
        PivotPointComponent cameraPivot = GetPivotPointComponent();
        TransformComponent playerTransform = GetTransformComponent(playerEntity);

        // 上下角度（Pitch）のクランプ（反転防止）
        if (cameraPivot.phi < minPhi) cameraPivot.phi = minPhi;
        if (cameraPivot.phi > maxPhi) cameraPivot.phi = maxPhi;

        // ピボット位置をプレイヤーへ追従（線形補間で滑らかに）
        Vector3 targetPos = playerTransform.position + targetOffset;
        float t = followSmoothness * dt;
        if (t > 1.0f) t = 1.0f;
        cameraPivot.pivotPoint += (targetPos - cameraPivot.pivotPoint) * t;

        // 球面座標からカメラの位置と回転を反映
        cameraTransform.position = cameraPivot.GetSphericalCoordinates();
        cameraTransform.rotation = cameraPivot.GetEulerAngles();

        // コンポーネントを適用（CameraComponentは変更していないため省略可）
        SetTransformComponent(cameraTransform);
        SetPivotPointComponent(cameraPivot);
	}
}
