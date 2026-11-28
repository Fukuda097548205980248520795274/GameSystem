class Sun
{
	// WorldTimeComponent を持つエンティティのキャッシュ
    private Entity worldTimeEntity;
    private bool hasWorldTime = false;

    // 太陽の調整用パラメータ
    private float latitudeTiltRad = 0.61f; // 軌道の傾き（約35度：日本付近のイメージ）
    private float maxSunIntensity = 1.2f;  // 昼の最大光量
    private float maxMoonIntensity = 0.2f; // 夜の最大光量（月光）

	Sun()
	{
		// コンストラクタ
	}

	void Initialize()
	{
		// "WorldTime" タグを持つエンティティを探してキャッシュ
        array<Entity>@ entities = FindEntitiesWithTag("WorldTime");
        if (entities !is null && entities.length() > 0)
        {
            worldTimeEntity = entities[0];
            hasWorldTime = true;
        }
        else
        {
            Log("Sun: 'WorldTime' タグを持つエンティティが見つかりません。");
        }
	}

	void Update()
	{
		if (!hasWorldTime) return;

        // 現在の時刻（0.0 ～ 24.0）を取得
        WorldTimeComponent worldTime = GetWorldTimeComponent(worldTimeEntity);
        float hour = worldTime.currentHour;

		
        // --------------------------------------------------
        // 1. 太陽の位置方向ベクトル（地面 → 太陽）を計算
        //    6:00 = 0 (東・日の出) / 12:00 = π/2 (南中) / 18:00 = π (西・日没)
        // --------------------------------------------------
        const float PI = 3.14159265f;
        float angle = ((hour - 6.0f) / 24.0f) * 2.0f * PI;

        float sinA = sin(angle);
        float cosA = cos(angle);

        // 東(+X)から昇り、南(-Z)へ傾きながら西(-X)へ沈む軌道
        Vector3 sunPosDir;
        sunPosDir.x = cosA;
        sunPosDir.y = sinA * cos(latitudeTiltRad);
        sunPosDir.z = -sinA * sin(latitudeTiltRad);
        sunPosDir = sunPosDir.Normalize();

        // 太陽の高度（-1.0 ～ 1.0。0以上が昼、0未満が夜）
        float elevation = sunPosDir.y;


        // --------------------------------------------------
        // 2. 光の向き・色・強さを決定（太陽と月を1灯で切り替え）
        // --------------------------------------------------
        DirectionalLightComponent light = GetDirectionalLightComponent();

        if (elevation >= 0.0f)
        {
            // 【昼：太陽光】光の進行方向（太陽 → 地面）なので反転
            light.direction = sunPosDir * -1.0f;
            light.color     = CalcSunColor(elevation);
            light.intensity = SmoothStep(0.0f, 0.25f, elevation) * maxSunIntensity;
        }
        else
        {
            // 【夜：月光】太陽の正反対（月）から地面へ向かう方向
            float moonElevation = -elevation;
            light.direction = sunPosDir; // (-sunPosDir) をさらに反転するのでそのまま sunPosDir
            light.color     = Vector3(0.45f, 0.60f, 0.85f);
            light.intensity = SmoothStep(0.0f, 0.25f, moonElevation) * maxMoonIntensity;
        }

        SetDirectionalLightComponent(light);
	}


	// 太陽高度（0.0 ～ 1.0）から太陽光の色を計算
    private Vector3 CalcSunColor(float elevation)
    {
        Vector3 horizonColor = Vector3(1.0f, 0.35f, 0.12f); // 日の出・日没の赤オレンジ
        Vector3 goldenColor  = Vector3(1.0f, 0.72f, 0.45f); // 朝焼け・夕焼けの黄金色
        Vector3 noonColor    = Vector3(1.0f, 0.96f, 0.90f); // 昼の自然な白

        if (elevation < 0.2f)
        {
            float t = Clamp(elevation / 0.2f, 0.0f, 1.0f);
            return LerpVec3(horizonColor, goldenColor, t);
        }
        else
        {
            float t = Clamp((elevation - 0.2f) / 0.35f, 0.0f, 1.0f);
            return LerpVec3(goldenColor, noonColor, t);
        }
    }

    // 値を minVal ～ maxVal の範囲に制限
    private float Clamp(float v, float minVal, float maxVal)
    {
        if (v < minVal) return minVal;
        if (v > maxVal) return maxVal;
        return v;
    }

	// 線形補間（0.0 ～ 1.0）を滑らかにする関数
    private float SmoothStep(float edge0, float edge1, float x)
    {
        float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

	// 線形補間（0.0 ～ 1.0）で Vector3 を補間
    private Vector3 LerpVec3(const Vector3 &in a, const Vector3 &in b, float t)
    {
        return a + (b - a) * t;
    }
}
