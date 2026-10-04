#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <utility>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <ImGuizmo.h>
#include <imnodes.h>

#include "Vector/Vector2/Vector2.h"
#include "RenderContext/DXHeap/DXHeap.h"

namespace Detail
{
	class WinApp;
	class DXSwapChain;
	class OffscreenResource;
	
	class ImGuiRender
	{
	public:

		/// @brief デストラクタ
		~ImGuiRender();

		/// @brief 初期化
		/// @param device 
		/// @param winApp 
		/// @param heap 
		/// @param swapChain 
		void Initialize(ID3D12Device* device, WinApp* winApp, DXHeap* heap, DXSwapChain* swapChain);

		/// @brief リサイズ
		/// @param width 
		/// @param height 
		void Resize(int32_t width, int32_t height);

		/// @brief フレーム開始
		void FrameStart();

		/// @brief Dockスペースを作成する
		void CreateDockSpace();

		/// @brief ImGuiスクリーンを描画する
		/// @param resource 
		/// @param gpuHandle 
		/// @param commandList 
		void DrawImGuiScreen(OffscreenResource* currentOffscreen, ID3D12GraphicsCommandList* commandList, int32_t frameIndex);

		/// @brief ビューウィンドウ内のカーソルの位置を取得する
		/// @return 
		Vector2 GetViewWindowCursorPos()const { return viewWindowCursorPos_; }

		/// @brief ビューウィンドウ内にカーソルがホバーしているかどうか
		/// @return 
		bool IsViewWindowHover()const { return isViewWindowHover_; }


	private:


		// SRVハンドル
		SRVDescriptorHandle srvHandle_;


		// スクリーン横幅
		float screenWidth_ = 0.0f;

		// スクリーン縦幅
		float screenHeight_ = 0.0f;


	private:

		// ウィンドウアプリケーション
		WinApp* winApp_ = nullptr;

		/// @brief ビューウィンドウ内のカーソル位置
		Vector2 viewWindowCursorPos_ = Vector2(0.0f, 0.0f);

		/// @brief ビューウィンドウにホバーしているかどうか
		bool isViewWindowHover_ = false;
	};
}