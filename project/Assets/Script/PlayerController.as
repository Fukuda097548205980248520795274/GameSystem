class PlayerController
{
	// コンストラクタ
	PlayerController()
	{

	}

	// 初期化
	void Initialize()
	{
		
	}

	// 更新処理
	void Update(float moveSpeed, Entity playerCamera)
	{
		// 移動入力を取得
		Vector2 inputMove = InputMove();

		// 移動入力がない場合は処理を抜ける
		if (inputMove.x == 0.0f && inputMove.y == 0.0f)
		{
			return;
		}

		// プレイヤーとカメラのコンポーネントを取得
		TransformComponent playerTransform = GetTransformComponent();
		TransformComponent cameraTransform = GetTransformComponent(playerCamera);
		PivotPointComponent cameraPivot = GetPivotPointComponent(playerCamera);

		// カメラの向いている方向（カメラ位置 -> 注視点）を計算
		Vector3 cameraForward = cameraPivot.pivotPoint - cameraTransform.position;
		cameraForward.y = 0.0f;

		// 真上からの見下ろし等で水平成分が0になった場合の保険
		if (cameraForward.x == 0.0f && cameraForward.z == 0.0f)
		{
			cameraForward = Vector3(0.0f, 0.0f, 1.0f);
		}
		else
		{
			cameraForward = cameraForward.Normalize();
		}

		// カメラの右方向ベクトル（Y軸上方向とcameraForwardの外積）
		Vector3 cameraRight = Vector3(cameraForward.z, 0.0f, -cameraForward.x);

		// カメラ基準の移動方向を合成して正規化
		Vector3 moveDir = (cameraForward * inputMove.y + cameraRight * inputMove.x).Normalize();

		// 位置を更新
		playerTransform.position += moveDir * moveSpeed * GetDeltaTime();
		SetTransformComponent(playerTransform);
	}

	// 移動入力
	Vector2 InputMove()
	{
		Vector2 move = Vector2(0.0f, 0.0f);

		// ゲームパッド操作
		if(Input::IsGamepadEnable())
		{
			// 左スティックの入力を取得
			move = Input::GetGamepadLeftStick();
		}
		else
		{
			// キーボード操作

			// W/S（前後）
			if (Input::GetKeyPress(Key::W)) { move.y += 1.0f; }
			if (Input::GetKeyPress(Key::S)) { move.y -= 1.0f; }

			// A/D（左右）
			if (Input::GetKeyPress(Key::A)) { move.x -= 1.0f; }
			if (Input::GetKeyPress(Key::D)) { move.x += 1.0f; }
		}

		return move;
	}
}
