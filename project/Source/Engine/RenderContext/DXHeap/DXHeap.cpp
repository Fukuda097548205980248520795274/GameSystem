#include "DXHeap.h"
#include <format>
#include <cassert>

#include "Engine.h"

/// @brief 初期化
/// @param device 
/// @param log 
void Detail::DXHeap::Initialize(ID3D12Device* device)
{
	// nullptrチェック
	assert(device);

	// 引数を受け取る
	device_ = device;


	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();

	// RTVディスクリプタヒープの生成
	rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtvDescriptorNum_, false);
	if (engine)engine->Log(LogLevel::Info, "RTVディスクリプタヒープ生成");

	// SRVディスクリプタヒープの生成
	srvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvDescriptorNum_, true);
	if (engine)engine->Log(LogLevel::Info, "SRVディスクリプタヒープ生成");

	// DSVディスクリプタヒープの生成
	dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, dsvDescriptorNum_, false);
	if (engine)engine->Log(LogLevel::Info, "DSVディスクリプタヒープ生成");

	// ディスクリプタサイズを取得
	rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	srvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	dsvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

/// @brief RTV用ハンドルを取得する
/// @return 
D3D12_CPU_DESCRIPTOR_HANDLE Detail::DXHeap::GetRtvDescriptorHandle()
{
	// RTVディスクリプタの最大数を超過していないか確認
	assert(useRtvDescriptor_ < static_cast<int>(rtvDescriptorNum_) && "RTVディスクリプタの最大数を超過しています");

	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();

	// CPUハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += rtvDescriptorSize_ * useRtvDescriptor_;

	// ログを出力する
	if (engine)
	{
		engine->Log(LogLevel::Info, std::format("ビュー : RTV, ハンドル : CPU, 値 : {}, ポインタ : {}", useRtvDescriptor_, cpuHandle.ptr));
	}

	// 数をカウントする
	useRtvDescriptor_++;

	return cpuHandle;
}

/// @brief SRV用ハンドルを取得する
/// @return 
Detail::SRVDescriptorHandle Detail::DXHeap::GetSrvDescriptorHandle()
{
	// SRVディスクリプタの最大数を超過していないか確認
	assert(useSrvDescriptor_ < static_cast<int>(srvDescriptorNum_) && "SRVディスクリプタの最大数を超過しています");

	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();

	// CPUハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += srvDescriptorSize_ * useSrvDescriptor_;

	// GPUハンドルの取得
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += srvDescriptorSize_ * useSrvDescriptor_;

	// ログを出力する
	if (engine)
	{
		engine->Log(LogLevel::Info, std::format("ビュー : SRV, ハンドル : CPU, 値 : {}, ポインタ : {}", useSrvDescriptor_, cpuHandle.ptr));
		engine->Log(LogLevel::Info, std::format("ビュー : SRV, ハンドル : GPU, 値 : {}, ポインタ : {}", useSrvDescriptor_, gpuHandle.ptr));
	}

	// 数をカウントする
	useSrvDescriptor_++;

	return { cpuHandle, gpuHandle };
}


/// @brief DSV用CPUハンドルを取得する
/// @return 
D3D12_CPU_DESCRIPTOR_HANDLE Detail::DXHeap::GetDsvDescriptorHandle()
{
	// DSVディスクリプタの最大数を超過していないか確認
	assert(useDsvDescriptor_ < static_cast<int>(dsvDescriptorNum_) && "DSVディスクリプタの最大数を超過しています");

	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();

	// CPUハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += dsvDescriptorSize_ * useDsvDescriptor_;

	// ログを出力する
	if (engine)
	{
		engine->Log( LogLevel::Info, std::format("ビュー : DSV, ハンドル : CPU, 値 : {}, ポインタ : {}", useDsvDescriptor_, cpuHandle.ptr));
	}

	// 数をカウントする
	useDsvDescriptor_++;

	return cpuHandle;
}


/// @brief ディスクリプタヒープを生成する
/// @param heapType 
/// @param descriptorNum 
/// @param shaderVisible 
/// @return 
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Detail::DXHeap::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorNum, bool shaderVisible)
{
	// エンジンのインスタンスを取得
	auto engine = Engine::GetInstance();

	// ディスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;

	// ディスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType; // ヒープの種類
	descriptorHeapDesc.NumDescriptors = descriptorNum; // ディスクリプタの数
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// 生成
	HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	if (!SUCCEEDED(hr))
	{
		// ディスクリプタヒープ生成失敗のログ
		if (engine)engine->Log(LogLevel::Error, "ディスクリプタヒープの生成に失敗しました");
		throw std::runtime_error("ディスクリプタヒープの生成に失敗しました");
	}

	// ログ出力
	if (engine)
	{
		if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		{
			engine->Log(LogLevel::Info, std::format("ヒープの種類 : RTV, ディスクリプタ数 : {}, シェーダ有効 : {}", descriptorNum, shaderVisible));
		}
		else if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		{
			engine->Log(LogLevel::Info, std::format("ヒープの種類 : CBV_SRV_UAV, ディスクリプタ数 : {}, シェーダ有効 : {}", descriptorNum, shaderVisible));
		}
		else if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		{
			engine->Log(LogLevel::Info, std::format("ヒープの種類 : DSV, ディスクリプタ数 : {}, シェーダ有効 : {}", descriptorNum, shaderVisible));
		}
	}

	return descriptorHeap;
}