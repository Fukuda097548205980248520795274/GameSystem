#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>


namespace Detail
{
	class DXCore
	{
	public:

		/// @brief コンストラクタ
		DXCore();

		/// @brief デストラクタ
		~DXCore() = default;

		/// @brief デバイスを取得する
		/// @return 
		ID3D12Device* GetDevice()const { return device_.Get(); }

		/// @brief DXGIファクトリーを取得する
		/// @return 
		IDXGIFactory7* GetDXGIFactory()const { return dxgiFactory_.Get(); }

		// Microsoft::WRL 省略
		template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;


	private:

		// DXGIファクトリー
		ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;

		// アダプタ
		ComPtr<IDXGIAdapter4> useAdapter_ = nullptr;

		// デバイス
		ComPtr<ID3D12Device> device_ = nullptr;
	};
}