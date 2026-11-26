#include "PlayerController.as"
#include "PlayerHealth.as"

class Player
{
	int testNum;

	private PlayerHealth health;

	Player()
	{
		// コンストラクタ
	}

	void Initialize()
	{
		// 初期化処理

		health.Initialize();
	}

	void Update()
	{
		// 更新処理

		health.Update();
	}
}
