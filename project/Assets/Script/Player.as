#include "PlayerController.as"
#include "PlayerHealth.as"

class Player
{
	// 移動速度
	float moveSpeed_ = 1.0f;

	// 初期の現在の体力
	int currentHp_ = 100;

	// 初期最大HP
	int maxHp_ = 100;

	// 体力
	private PlayerHealth health;

	// プレイヤー
	Player()
	{

	}

	// 初期化
	void Initialize()
	{
		// 体力の初期化
		health.Initialize(currentHp_, maxHp_);
	}

	// 更新処理
	void Update()
	{
		// 最大体力を越えないようにする
		if(currentHp_ >= maxHp_){currentHp_ = maxHp_;}

		// 体力の値を渡す
		health.SetCurrentHP(currentHp_);
		health.SetMaxHP(maxHp_);
	}
}
