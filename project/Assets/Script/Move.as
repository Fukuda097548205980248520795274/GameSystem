class Move
{
	// ターゲットエンティティ
	Entity targetEntity;

	int testIntNum;
	float testFloatNum;
	bool testBool;

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
		
    	transform.position.z += testFloatNum * GetDeltaTime();
    
    	SetTransformComponent(transform);
	}
}
