#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>
#include <vector>

namespace Detail
{
	// @brief SRV用ディスクリプタハンドル
	struct SRVDescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	};

	class DXHeap
	{
	public:

		/// @brief 初期化
		/// @param device 
		/// @param log 
		void Initialize(ID3D12Device* device);

		/// @brief RTV用ディスクリプタヒープを取得する
		/// @return 
		ID3D12DescriptorHeap* GetRtvDescriptorHeap()const { return rtvDescriptorHeap_.Get(); }

		/// @brief SRV用ディスクリプタヒープを取得する
		/// @return 
		ID3D12DescriptorHeap* GetSrvDescriptorHeap()const { return srvDescriptorHeap_.Get(); }

		/// @brief DSV用ディスクリプタヒープを取得する
		/// @return 
		ID3D12DescriptorHeap* GetDsvDescriptorHeap()const { return dsvDescriptorHeap_.Get(); }



		/// @brief RTV用ハンドルを取得する
		/// @return 
		D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDescriptorHandle();

		/// @brief SRV用ハンドルを取得する
		/// @return 
		SRVDescriptorHandle GetSrvDescriptorHandle();

		/// @brief DSV用ハンドルを取得する
		/// @return 
		D3D12_CPU_DESCRIPTOR_HANDLE GetDsvDescriptorHandle();



		/// @brief RTV用ディスクリプタハンドルを解放する
		/// @param handle 
		void FreeRtvDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle);

		/// @brief SRV用ディスクリプタハンドルを解放する
		/// @param index 
		void FreeSrvDescriptorHandle(const SRVDescriptorHandle& handle);

		/// @brief DSV用ディスクリプタハンドルを解放する
		/// @param handle 
		void FreeDsvDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle);


		// Microsoft::WRL 省略
		template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;


	private:

		/// @brief ディスクリプタヒープを生成する
		/// @param heapType 
		/// @param descriptorNum 
		/// @param shaderVisible 
		/// @return 
		ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorNum, bool shaderVisible);

		// デバイス
		ID3D12Device* device_ = nullptr;


	private:

		// ディスクリプタサイズ
		UINT rtvDescriptorSize_ = 0;
		UINT srvDescriptorSize_ = 0;
		UINT dsvDescriptorSize_ = 0;


	private:

		// RTV用ディスクリプタヒープ
		ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_ = nullptr;

		// RTV用ディスクリプタ数
		UINT rtvDescriptorNum_ = 128;

		// RTV用ディスクリプタ使用数
		int32_t useRtvDescriptor_ = 0;

		/// @brief RTV用ディスクリプタの空きインデックス
		std::vector<uint32_t> freeRtvIndices_;


	private:

		// SRV用ディスクリプタヒープ
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_ = nullptr;

		// SRV用ディスクリプタ数
		UINT srvDescriptorNum_ = 1024;

		// SRV用ディスクリプタ使用数
		int32_t useSrvDescriptor_ = 0;

		/// @brief SRV用ディスクリプタの空きインデックス
		std::vector<uint32_t> freeSrvIndices_;


	private:

		// DSV用ディスクリプタヒープ
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_ = nullptr;

		// DSV用ディスクリプタ数
		UINT dsvDescriptorNum_ = 8;

		// DSV用ディスクリプタ使用数
		int32_t useDsvDescriptor_ = 0;

		/// @brief DSV用ディスクリプタの空きインデックス
		std::vector<uint32_t> freeDsvIndices_;
	};
}