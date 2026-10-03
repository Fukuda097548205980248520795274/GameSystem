#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>
#include <vector>

namespace Detail
{
	class DXHeap;
	class WinApp;
	class DXCore;
	class DXCommand;
	class DXFence;

	class DXSwapChain
	{
	public:

		/// @brief コンストラクタ
		/// @param heap 
		/// @param winApp 
		/// @param dxCore 
		/// @param dxCommand 
		DXSwapChain(DXHeap* heap, WinApp* winApp, DXCore* dxCore, DXCommand* dxCommand) { Initialize(heap, winApp, dxCore, dxCommand); }

		/// @brief デストラクタ
		~DXSwapChain() = default;

		/// @brief スワップチェーンの表示
		/// @param syncInterval 
		/// @param flags 
		void Present(int syncInterval = 0, UINT flags = 0) { swapChain_->Present(syncInterval, flags); }

		/// @brief サイズを作り直す
		/// @param device 
		/// @param width 
		/// @param height 
		void Resize(ID3D12Device* device, int32_t width, int32_t height);

		/// @brief スワップチェーンの設定の取得
		/// @return 
		DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc()const { return swapChainDesc_; }

		/// @brief 現在のバックバッファのインデックスを取得する
		/// @return 
		UINT GetCurrentBackBufferIndex() const { return swapChain_->GetCurrentBackBufferIndex(); }

		/// @brief スワップチェーンRTVのハンドルの取得
		/// @param index 
		/// @return 
		D3D12_CPU_DESCRIPTOR_HANDLE GetSwapChainRtvHandle(UINT index)const { return rtvCPUHandle_[index]; }

		/// @brief スワップチェーンのリソースを取得する
		/// @param index 
		/// @return 
		ID3D12Resource* GetSwapChainResource(UINT index)const { return swapChainResource_[index].Get(); }

		/// @brief RTVの設定を取得する
		/// @return 
		D3D12_RENDER_TARGET_VIEW_DESC GetRtvDesc()const { return rtvDesc_; }


		// Microsoft::WRL 省略
		template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	private:


		/// @brief 初期化
		/// @param heap 
		/// @param winApp 
		/// @param dxCore
		/// @param dxCommand
		void Initialize(DXHeap* heap, WinApp* winApp, DXCore* dxCore, DXCommand* dxCommand);


	private:


		// スワップチェーンの設定
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};

		// スワップチェーン
		ComPtr<IDXGISwapChain4> swapChain_ = nullptr;

		// スワップチェインのリソース
		std::vector<ComPtr<ID3D12Resource>> swapChainResource_;

		// RTVのCPUハンドル
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvCPUHandle_;


		// RTV
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
	};
}