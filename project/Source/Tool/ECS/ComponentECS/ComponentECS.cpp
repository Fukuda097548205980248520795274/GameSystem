#include "ComponentECS.hpp"
#include "Engine.hpp"

/// @brief コンストラクタ
TextureComponent::TextureComponent()
{
	auto renderContext = Engine::GetInstance()->GetRenderContext();
	if (!renderContext)throw std::runtime_error("レンダーコンテキストが取得できませんでした");

	// デフォルトのキューブマップを作成
	handle = renderContext->GetTextureStore()->GetDefaultTextureHandle();
}

/// @brief コンストラクタ
CubemapComponent::CubemapComponent()
{
	auto renderContext = Engine::GetInstance()->GetRenderContext();
	if (!renderContext)throw std::runtime_error("レンダーコンテキストが取得できませんでした");

	// デフォルトのキューブマップを作成
	handle = renderContext->GetTextureStore()->GetDefaultCubemapHandle();
}