#pragma once
#include "DXCore/DXCore.h"
#include "DXCommand/DXCommand.h"
#include "DXFence/DXFence.h"
#include "DXHeap/DXHeap.h"
#include "DXSwapChain/DXSwapChain.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "MultiPass/MultiPass.h"
#include <memory>

namespace Detail
{
	class WinApp;

	class RenderContext
	{
	public:

		/// @brief コンストラクタ
		RenderContext() = default;

		/// @brief デストラクタ
		~RenderContext();

		/// @brief 初期化
		/// @param winApp 
		void Initialize(WinApp* winApp);

		/// @brief 最大バッファ数を取得する
		/// @return 
		uint32_t GetMaxBufferCount() const { return kBufferCount; }

		/// @brief フレームインデックスを取得する
		/// @return 
		uint32_t GetFrameIndex() const { return frameIndex_; }

		/// @brief シーン前処理
		void NewFrame();

		/// @brief 描画前処理
		void PreDraw();

		/// @brief 描画後処理
		void PostDraw();

		/// @brief レンダーパスを作成する
		/// @param priority 
		/// @param blendMode 
		/// @param drawFunc 
		/// @return 
		Entity CreatePass(int priority, BlendMode blendMode, std::function<void()> drawFunc) { return multiPass_->CreatePass(priority, blendMode, drawFunc); }


	private:

		/// @brief DXCore
		std::unique_ptr<DXCore> core_ = nullptr;

		/// @brief DXCommand
		std::unique_ptr<DXCommand> command_ = nullptr;

		/// @brief DXFence
		std::unique_ptr<DXFence> fence_ = nullptr;

		/// @brief DXHeap
		std::unique_ptr<DXHeap> heap_ = nullptr;

		/// @brief DXSwapChain
		std::unique_ptr<DXSwapChain> swapChain_ = nullptr;

		/// @brief シェーダコンパイラ
		std::unique_ptr<ShaderCompiler> shaderCompiler_ = nullptr;

		/// @brief マルチパス
		std::unique_ptr<MultiPass> multiPass_ = nullptr;


	private:

		/// @brief ビューポート
		D3D12_VIEWPORT viewport_{};

		/// @brief シザー矩形
		D3D12_RECT scissorRect_{};


	private:

		/// @brief バッファ数
		static constexpr uint32_t kBufferCount = 2;

		/// @brief 現在のフレームインデックス
		uint32_t frameIndex_ = 0;
	};
}