#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>
#include <vector>

namespace Detail
{
	class DXFence
	{
	public:

		/// @brief コンストラクタ
		~DXFence();

		/// @brief 初期化
		/// @param device 
		void Initialize(ID3D12Device* device);

		/// @brief GPUにシグナルを送る
		/// @param commandQueue 
		void SendSignal(ID3D12CommandQueue* commandQueue);

		/// @brief GPUの処理が完了するまで待機する
		void WaitGPU();

		// Microsoft::WRL 省略
		template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;


	private:

		// フェンス
		ComPtr<ID3D12Fence> fence_ = nullptr;

		// フェンスの値
		uint64_t currentFenceValue_ = 0;

		// フェンスの値を保持する配列
		std::vector<uint64_t> fenceValues_;

		// フェンスのイベント
		HANDLE fenceEvent_;
	};
}