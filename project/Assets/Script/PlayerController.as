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
	void Update()
	{
		// 移動入力を取得
		Vector2 inputMove = InputMove();

		// 移動入力がない場合は処理を抜ける
		if(inputMove.x == 0.0f && inputMove.y == 0.0f)return;

		// 移動入力を3Dベクトルに変換
		Vector3 inputMove3D = Vector3(inputMove.x, 0.0f, inputMove.y);
		inputMove3D = inputMove3D.Normalize();

		// TransformComponentを取得
		TransformComponent transform = GetTransformComponent();
		transform.position += inputMove3D * 1.0f * GetDeltaTime();

		// TransformComponentを更新
		SetTransformComponent(transform);
	}

	// 移動入力
	Vector2 InputMove()
	{
		// 移動ベクトル
		Vector2 move = Vector2();

		// Wキーが押されたら奥に移動
		if(Input::GetKeyPress(Key::W))
		{
			move.y = 1.0f;
		}

		// Aキーが押されたら左に移動
		if(Input::GetKeyPress(Key::A))
		{
			move.x = -1.0f;
		}

		// Sキーが押されたら手前に移動
		if(Input::GetKeyPress(Key::S))
		{
			move.y = -1.0f;
		}

		// Dキーが押されたら右に移動
		if(Input::GetKeyPress(Key::D))
		{
			move.x = 1.0f;
		}

		return move;
	}
}
