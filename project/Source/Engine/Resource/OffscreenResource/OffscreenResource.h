#pragma once
#include <vector>
#include <string>

#include "RenderContext/DXHeap/DXHeap.h"

namespace Detail
{
	class DXHeap;

	class OffscreenResource
	{
	public:

		/// @brief コンストラクタ
		/// @param device 
		/// @param heap 
		/// @param width 
		/// @param height 
		OffscreenResource(ID3D12Device* device, DXHeap* heap, int32_t width, int32_t height) { Initialize(device, heap, width, height); }

		/// @brief デストラクタ
		~OffscreenResource() = default;

		/// @brief サイズを作り直す
		/// @param device 
		/// @param width 
		/// @param height 
		void Resize(ID3D12Device* device, int32_t width, int32_t height);

		/// @brief バリアを張る
		/// @param commandList 
		/// @param before 
		/// @param after 
		/// @param frameIndex
		void Barrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t frameIndex);

		/// @brief レンダーターゲットの設定とクリア
		/// @param commandList 
		/// @param dsvHandle 
		/// @param frameIndex
		void ClearRenderTarget(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, uint32_t frameIndex);

		/// @brief レンダーターゲットの設定
		/// @param commandList 
		/// @param dsvHandle 
		/// @param frameIndex
		void SetRenderTarget(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex
		void RegisterGraphics(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief コマンドリストに登録する
		/// @param commandList 
		/// @param rootParameterIndex 
		/// @param frameIndex
		void RegisterCompute(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, uint32_t frameIndex);

		/// @brief RTV用ハンドルを取得する
		/// @return 
		D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(uint32_t frameIndex) { return rtvHandle_[frameIndex]; }

		/// @brief SRV用ハンドルを取得する
		/// @return 
		SRVDescriptorHandle GetSrvHandle(uint32_t frameIndex) { return srvHandle_[frameIndex]; }

		/// @brief リソースを取得する
		/// @return 
		ID3D12Resource* GetResource(uint32_t frameIndex) { return resource_[frameIndex].Get(); }

		/// @brief 名前を取得する
		/// @return 
		const std::string& GetName() { return name_; }

		/// @brief 名前を設定する
		/// @param name 
		void SetName(const std::string& name) { name_ = name; }


		template<typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;


	private:

		/// @brief 初期化
		/// @param device 
		/// @param heap 
		/// @param width
		/// @param height
		void Initialize(ID3D12Device* device, DXHeap* heap, int32_t width, int32_t height);

		// リソース
		std::vector<ComPtr<ID3D12Resource>> resource_;

		// RTV用CPUハンドル
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandle_{};

		// SRV用ハンドル
		std::vector<SRVDescriptorHandle> srvHandle_{};

		/// @brief 名前
		std::string name_{};
	};
}