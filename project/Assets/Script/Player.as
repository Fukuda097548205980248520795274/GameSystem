#include "PlayerController.as"
#include "PlayerHealth.as"

class Player
{
	// 移動速度
	float moveSpeed_ = 1.0f;

	// 初期HP
	int currentHp_ = 100;

	// 初期最大HP
	int maxHp_ = 100;

	// プレイヤー用カメラ
	Entity playerCamera_;


	// 体力
	private PlayerHealth health_;

	// コントローラ
	private PlayerController controller_;


	// プレイヤー
	Player()
	{

	}

	// 初期化
	void Initialize()
	{
		// 体力の初期化
		maxHp_ = health_.ClampMaxHP(maxHp_);
		currentHp_ = health_.ClampHP(currentHp_, maxHp_);

		// コントローラの初期化
		controller_.Initialize();
	}

	// 更新処理
	void Update()
	{
		// インスペクタでの直接編集も含め、常に有効範囲内に保つ
		maxHp_ = health_.ClampMaxHP(maxHp_);
		currentHp_ = health_.ClampHP(currentHp_, maxHp_);

		// 死亡している場合は操作を受け付けない
		if (health_.IsDead(currentHp_))
		{
			return;
		}

		// コントローラの更新
		controller_.Update(moveSpeed_, playerCamera_);
	}

	// ダメージを受ける
	void TakeDamage(int amount)
	{
		currentHp_ = health_.TakeDamage(currentHp_, maxHp_, amount);
	}

	// 回復する
	void Heal(int amount)
	{
		currentHp_ = health_.Heal(currentHp_, maxHp_, amount);
	}
}
