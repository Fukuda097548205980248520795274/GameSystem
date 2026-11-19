class Move
{
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

    	transform.position.z += 0.1f;
    
    	SetTransformComponent(transform);
	}
}
