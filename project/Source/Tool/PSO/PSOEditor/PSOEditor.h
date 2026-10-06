#pragma once
#include "PSO/DynamicPSOBuilder/DynamicPSOBuilder.h"

namespace Detail
{
	class ShaderCompiler;

	struct PSOItem
	{
		// 名前
		std::string name = "新規 PSO";

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
		std::string statusMessage = "未 ビルド";
	};

	// 古いPSOやルートシグネチャを破棄するための構造体
	struct GarbageResource
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
		uint32_t frameAge = 0;
	};

	class PSOEditor
	{
	public:

		/// @brief コンストラクタ
		PSOEditor();

		/// @brief 更新処理
		void Update();

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

		/// @brief PSO設定をJSONファイルに保存する
		/// @param filepath 
		void SaveToFile(const std::string& filename);

		/// @brief JSONファイルからPSO設定を読み込む
		/// @param filepath 
		void LoadFromFile(const std::string& filename);


	private:

		/// @brief 選択されているPSOファイルの名前を取得する
		/// @return 
		std::string GetSelectedPsoFileName() const;

		/// @brief 選択されているPSOファイルのパスを取得する
		/// @return 
		std::string GetSelectedPsoFilePath() const;

		/// @brief PSOファイルリストを更新する
		void RefreshPsoFileList();

		/// @brief シェーダファイルリストを更新する
		void RefreshShaderFileList();


	private:

		// 複数のPSOをリストとして管理
		std::vector<PSOItem> psoItems_;

		// 現在エディタで選択しているPSOのインデックス
		int selectedIndex_ = -1;

		// 編集時の自動再構築を行うかどうかのフラグ
		bool isAutoRebuild_ = false;

		/// @brief 古いPSOやルートシグネチャを破棄するためのキュー
		std::vector<GarbageResource> garbageQueue_;


	private:

		/// @brief PSO設定の保存先ディレクトリ
		const std::string kDir = "./Assets/EngineData/PSO/";

		// PSOデータファイルリスト
		std::vector<std::string> psoFileList_;
		int selectedPsoFileIndex_ = -1;

		// 保存するファイル名のバッファ
		std::string saveFileNameBuffer_ = "";

		// コンボボックスの開閉状態を追跡するフラグ
		bool wasComboOpen_ = false;


	private:

		/// @brief シェーダファイルの走査対象ディレクトリ
		const std::string kShaderDir = "./Assets/Shader/";

		// シェーダーファイルリスト
		std::vector<std::wstring> shaderFileList_;
		std::vector<std::string> shaderFileListUtf8_;

		// シェーダープルダウン前フレーム開閉フラグ
		bool wasVsComboOpen_ = false;
		bool wasPsComboOpen_ = false;
		bool wasCsComboOpen_ = false;
	};
}