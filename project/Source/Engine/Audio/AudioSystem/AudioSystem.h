#pragma once
#include "../AudioStore/AudioStore.h"

namespace Detail
{
	/// @brief オーディオシステム
	class AudioSystem
	{
	public:

		/// @brief コンストラクタ
		AudioSystem() { Initialize(); }

		/// @brief デストラクタ
		~AudioSystem() = default;

		/// @brief 更新処理
		void Update();


	private:

		/// @brief 初期化
		void Initialize();

		/// @brief オーディオストア
		std::unique_ptr<AudioStore> audioStore_;
	};
}