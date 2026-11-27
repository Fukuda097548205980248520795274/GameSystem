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
	void Update(float moveSpeed)
	{
		// 移動入力を取得
		Vector2 inputMove = InputMove();

		// 移動入力がない場合は処理を抜ける
		if (inputMove.x == 0.0f && inputMove.y == 0.0f)
		{
			return;
		}

		// 移動入力を3Dベクトルに変換して正規化
		Vector3 moveDir = Vector3(inputMove.x, 0.0f, inputMove.y).Normalize();

		// TransformComponentを取得して位置を更新
		TransformComponent transform = GetTransformComponent();
		transform.position += moveDir * moveSpeed * GetDeltaTime();

		// TransformComponentを反映
		SetTransformComponent(transform);
	}

	// 移動入力
	Vector2 InputMove()
	{
		Vector2 move = Vector2(0.0f, 0.0f);

		// W/S（前後）
		if (Input::GetKeyPress(Key::W)) { move.y += 1.0f; }
		if (Input::GetKeyPress(Key::S)) { move.y -= 1.0f; }

		// A/D（左右）
		if (Input::GetKeyPress(Key::A)) { move.x -= 1.0f; }
		if (Input::GetKeyPress(Key::D)) { move.x += 1.0f; }

		return move;
	}
}
