#pragma once
#include "DXCore/DXCore.h"
#include <memory>

namespace Detail
{
	class RenderContext
	{
	public:

		/// @brief コンストラクタ
		RenderContext();

		/// @brief デストラクタ
		~RenderContext() = default;


	private:

		/// @brief DXCore
		std::unique_ptr<DXCore> core_ = nullptr;

	};
}