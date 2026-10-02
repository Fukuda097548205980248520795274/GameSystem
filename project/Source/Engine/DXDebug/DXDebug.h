#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

namespace Detail
{
	class DXDebug
	{
	public:

		/// @brief コンストラクタ
		DXDebug();

		/// @brief デストラクタ
		~DXDebug();

		/// @brief 警告・エラーで停止させる
		/// @param device 
		void Stop(ID3D12Device* device);

		// Microsoft::WRL 省略
		template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;


	private:

		// デバッグコントローラ
		ComPtr<ID3D12Debug1> debugController_ = nullptr;
	};
}