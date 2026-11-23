class Move
{
	// ターゲットエンティティ
	Entity targetEntity;

	int testIntNum;
	float testFloatNum;
	bool testBool;
	Vector2 testVector2;
	Vector3 testVector3;
	string testString;

	Move()
	{
		// コンストラクタ
	}

	void Initialize()
	{
		// 初期化処理
	}

	void Update()
	{
		// 更新処理

		// ヘルパーで生成された名前に合わせて呼び出す
    	TransformComponent transform = GetTransformComponent();
		
		// Wキーで移動する
		if (Input::GetKeyPress(Key::W))
		{
			transform.position.z += 2.0f * GetDeltaTime();
		}

		Matrix4x4 matrix4x4;
		Matrix3x3 matrix3x3;
    
    	SetTransformComponent(transform);
	}
}
