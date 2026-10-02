#include "RenderContext.h"

/// @brief コンストラクタ
Detail::RenderContext::RenderContext()
{
	// DXCoreを作成
	core_ = std::make_unique<DXCore>();
}