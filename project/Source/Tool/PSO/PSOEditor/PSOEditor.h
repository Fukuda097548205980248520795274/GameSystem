#pragma once
#include "PSO/DynamicPSOBuilder/DynamicPSOBuilder.h"

namespace Detail
{
	class ShaderCompiler;

	struct PSOItem
	{
		// 名前
		std::string name = "New PSO";

		// PSOの設定
		PSODescription desc;

		// PSOとルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso = nullptr;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig = nullptr;

		// 設定が変更されたかどうかのフラグ
		bool isDirty = true;

		// ビルドに失敗したかどうかのフラグ
		bool isBuildFailed = false;

		// ビルドステータスメッセージ
		std::string statusMessage = "Not Built";
	};

	class PSOEditor
	{
	public:

		/// @brief UIを描画する
		/// @param device 
		/// @param compiler 
		void DrawUI(ID3D12Device* device, ShaderCompiler* compiler);

		/// @brief 名前からアクティブなPSOを取得する
		/// @param name 
		/// @return 
		ID3D12PipelineState* GetPSO(const std::string& name) const;

		/// @brief 名前からアクティブなルートシグネチャを取得する
		/// @param name 
		/// @return 
		ID3D12RootSignature* GetRootSignature(const std::string& name) const;

		/// @brief PSOの数を取得する
		/// @return 
		size_t GetPSOCount() const { return psoItems_.size(); }

		/// @brief インデックスからPSOを取得する
		/// @param index 
		/// @return 
		ID3D12PipelineState* GetPSO(size_t index) const { return psoItems_[index].pso.Get(); }

		/// @brief インデックスからルートシグネチャを取得する
		/// @param index 
		/// @return 
		ID3D12RootSignature* GetRootSignature(size_t index) const { return psoItems_[index].rootSig.Get(); }


	private:

		// 複数のPSOをリストとして管理
		std::vector<PSOItem> psoItems_;

		// 現在エディタで選択しているPSOのインデックス
		int selectedIndex_ = -1;

		// 編集時の自動再構築を行うかどうかのフラグ
		bool isAutoRebuild_ = false;
	};
}