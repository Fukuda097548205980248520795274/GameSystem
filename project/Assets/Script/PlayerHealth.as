class PlayerHealth
{
	// 現在のHP
	private int currentHp_ = 0;

	// 最大HP
	private int maxHp_ = 0;

	// コンストラクタ
	PlayerHealth(){}

	// 初期化
	void Initialize(int startCurrentHp,int startMaxHp)
	{
		// 体力の設定
		currentHp_ = startCurrentHp;
		maxHp_ = startMaxHp;
	}

	// 更新処理
	void Update()
	{
		// 体力が越えないようにする
		if(currentHp_ >= maxHp_){currentHp_ = maxHp_;}
	}

	// 死亡したかどうか
	bool IsDead()
	{
		// 体力がなくなっていたら死亡
		if(currentHp_ <= 0)return true;

		return false;
	}

	// 現在の体力を設定する
	void SetCurrentHP(int currentHp){currentHp_ = currentHp; }

	// 最大体力を設定する
	void SetMaxHP(int maxHp){maxHp_ = maxHp;}
}
