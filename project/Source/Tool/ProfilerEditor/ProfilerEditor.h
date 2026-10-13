#pragma once

namespace Detail
{
	class ProfilerEditor
	{
    public:

        /// @brief コンストラクタ
        ProfilerEditor() = default;

        /// @brief デストラクタ
        ~ProfilerEditor() = default;

        /// @brief UIを描画する
        void DrawUI();
	};
}