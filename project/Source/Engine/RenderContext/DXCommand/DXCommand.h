#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>

namespace Detail
{
	class DXCommand
	{
	public:

		// コンストラクタ・デストラクタ
		DXCommand() = default;
		~DXCommand() = default;

		/// @brief 初期化
		/// @param device 
		void Initialize(ID3D12Device* device);

		/// @brief コマンドキューを取得する
		/// @return 
		ID3D12CommandQueue* GetCommandQueue()const { return commandQueue_.Get(); }

		/// @brief コマンドアロケータを取得する
		/// @return 
		ID3D12CommandAllocator* GetCommandAllocator()const;

		/// @brief コマンドリストを取得する
		/// @return 
		ID3D12GraphicsCommandList* GetCommandList()const { return commandList_.Get(); }

		// Microsoft::WRL 省略
		template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;


	private:


		// コマンドキュー
		ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;

		// コマンドアロケータ
		std::vector<ComPtr<ID3D12CommandAllocator>> commandAllocators_;

		// コマンドリスト
		ComPtr<ID3D12GraphicsCommandList> commandList_;
	};
}