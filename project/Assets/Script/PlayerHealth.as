class PlayerHealth
{
	// コンストラクタ
	PlayerHealth()
	{
	}

	// 更新処理
	void Update()
	{
	}

	// 最大HPを 1 以上に収める
	int ClampMaxHP(int maxHp) const
	{
		return (maxHp < 1) ? 1 : maxHp;
	}

	// HPを 0 ～ maxHp の範囲に収める
	int ClampHP(int currentHp, int maxHp) const
	{
		if (currentHp > maxHp) { return maxHp; }
		if (currentHp < 0)     { return 0; }
		return currentHp;
	}

	// ダメージ処理（計算後のHPを返す）
	int TakeDamage(int currentHp, int maxHp, int amount) const
	{
		if (amount <= 0) { return ClampHP(currentHp, maxHp); }
		return ClampHP(currentHp - amount, maxHp);
	}

	// 回復処理（計算後のHPを返す）
	int Heal(int currentHp, int maxHp, int amount) const
	{
		if (amount <= 0) { return ClampHP(currentHp, maxHp); }
		return ClampHP(currentHp + amount, maxHp);
	}

	// 死亡判定
	bool IsDead(int currentHp) const
	{
		return currentHp <= 0;
	}
}
