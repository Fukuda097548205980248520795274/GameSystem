#pragma once
#include "Store/HandleData/HandleData.hpp"
#include "ECS/EntityECS/EntityECS.hpp"
#include <unordered_map>
#include <queue>
#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptstdstring/scriptstdstring.h>
#include <string>
#include <filesystem>
#include <fstream>

namespace Detail
{
	// @brief スクリプトタグ
	struct ScriptTag {};

	// @brief スクリプトハンドル
	using ScriptHandle = Handle<ScriptTag>;


	/// @brief スクリプトマネージャ
	class ScriptManager
	{
	public:

		/// @brief コンストラクタ
		ScriptManager() {Initialize();}

		/// @brief デストラクタ
		~ScriptManager() { Shutdown(); }

		/// @brief スクリプトモジュールをビルドする
		/// @param moduleName 
		/// @param scriptClass 
		/// @param scriptPath 
		/// @return 
		ScriptHandle BuildScriptModule(const std::string& moduleName, const std::string& scriptClass, const std::string& scriptPath);

		/// @brief スクリプトエンジンを取得する
		/// @return 
		asIScriptEngine* GetScriptEngine() const { return scriptEngine_; }

		/// @brief インデックスからハンドルを取得する
		/// @param index 
		/// @return 
		ScriptHandle GetHandleByIndex(uint32_t index) const;

		/// @brief スクリプトを削除する
		/// @param handle 
		void Remove(ScriptHandle handle);

		/// @brief スクリプトの数を取得する
		/// @return 
		size_t GetScriptCount() const { return slots_.size(); }

		/// @brief スクリプトハンドルからスクリプトパスを取得する
		/// @param handle 
		/// @return 
		std::string GetFilePath(ScriptHandle handle) const;

		/// @brief スクリプトハンドルからモジュール名を取得する
		/// @param handle 
		/// @return 
		std::string GetModuleName(ScriptHandle handle) const;

		/// @brief スクリプトハンドルからスクリプトクラス名を取得する
		/// @param handle 
		/// @return 
		std::string GetScriptClass(ScriptHandle handle) const;

		/// @brief 変更されたスクリプトをリロードする
		void ReloadModifiedScripts();

        /// @brief エンティティからスクリプトオブジェクトを取得する
        /// @param targetEntity 
        /// @return 
        asIScriptObject* GetScriptFromEntity(Entity targetEntity);

        /// @brief スクリプトのメタデータのみを登録する（実データはロードしない）
        /// @param moduleName 
        /// @param scriptClass 
        /// @param scriptPath 
        /// @return 
        ScriptHandle RegisterScriptMetadata(const std::string& moduleName, const std::string& scriptClass, const std::string& scriptPath);

        /// @brief 指定ハンドルのスクリプトが未ロードであればロードする（遅延ロード）
        /// @param handle 
        /// @return 
        bool EnsureLoaded(ScriptHandle handle);

        /// @brief スクリプトがロード済みか確認する
        /// @param handle 
        /// @return 
        bool IsLoaded(ScriptHandle handle) const;


	private:

		/// @brief 初期化
		void Initialize();

		/// @brief シャットダウン
		void Shutdown();

		/// @brief メッセージコールバック
		/// @param msg 
		void MessageCallback(const asSMessageInfo* msg);

		/// @brief スクリプトエンジンを登録する
		void RegisterEngineAPI();

        /// @brief 事前定義されたスクリプトファイルをエクスポートする
        /// @param outputPath 
        void ExportPredefinedFile(const std::string& outputPath);

        /// @brief フォルダを走査してスクリプトのメタデータのみを登録する
        void ScanScriptDirectory();


	private:

		/// @brief スクリプトエンジン
		asIScriptEngine* scriptEngine_ = nullptr;


		// スロット内部情報
		struct Slot
		{
			// スロットの世代番号
			uint32_t generation = 1;

			// スロットが使用中かどうか
			bool inUse = false;

            // 実データ（モジュール）がロード済みか
            bool isLoaded = false;

            // スクリプトファイルのパス
            std::string filePath = "";

			// モジュール名
			std::string moduleName = "";

			// スクリプトクラス名
			std::string scriptClass = "";

			// スクリプトファイルの最終更新時刻
			std::filesystem::file_time_type lastWriteTime;

            // スクリプトが依存しているインクルードファイルのパス
            std::vector<std::string> includedFiles;
		};

        // @brief バイナリファイルの識別用マジックナンバー
        static constexpr uint32_t kBinMagic = 0x53435232;

		// データテーブル
		std::vector<Slot> slots_;

		// 空きスロットのインデックスキュー
		std::queue<uint32_t> freeSlots_;

		// キャッシュ
		std::unordered_map<std::string, ScriptHandle> cache_;


	private:

        // スクリプトの格納ディレクトリ
        const std::string kScriptDir = "./Assets/Script/";

		// 中間データ用ディレクトリ
		const std::string kCookedDir = "./Cooked/Script/";


        // リロードタイマー
        float reloadTimer_ = 0.0f;

        // リロードが保留中かどうかのフラグ
        bool isPendingReload_ = false;

        // リロードのクールダウン時間（秒）
        const float reloadCooldown_ = 0.5f;
	};



	// バイトコードのストリームクラス
	class CBytecodeStream : public asIBinaryStream
	{
	public:

		/// @brief コンストラクタ
		/// @param path 
		/// @param write 
		CBytecodeStream(const std::string& path, bool write)
		{
			if (write)
			{
				file_.open(path, std::ios::binary | std::ios::out);
			}
			else
			{
				file_.open(path, std::ios::binary | std::ios::in);
			}
		}

		/// @brief デストラクタ
		~CBytecodeStream() { if (file_.is_open()) file_.close(); }

		/// @brief 読み込み
		/// @param ptr 
		/// @param size 
		/// @return 
		int Read(void* ptr, asUINT size) override
		{
            if (!file_.is_open()) return -1;
            file_.read(static_cast<char*>(ptr), size);
            return file_.good() ? 0 : -1;
		}

		/// @brief 書き込み
		/// @param ptr 
		/// @param size 
		/// @return 
		int Write(const void* ptr, asUINT size) override
		{
            if (!file_.is_open()) return -1;
            file_.write(static_cast<const char*>(ptr), size);
            return file_.good() ? 0 : -1;
		}

		/// @brief ファイルが開いているかどうかを確認する
		/// @return 
		bool IsOpen() const { return file_.is_open(); }

        /// @brief uint32_t を書き込む
        /// @param value 
        /// @return 
        bool WriteUint32(uint32_t value)
        {
            if (!file_.is_open()) return false;
            file_.write(reinterpret_cast<const char*>(&value), sizeof(value));
            return file_.good();
        }

        /// @brief uint32_t を読み込む
        /// @param outValue 
        /// @return 
        bool ReadUint32(uint32_t& outValue)
        {
            if (!file_.is_open()) return false;
            file_.read(reinterpret_cast<char*>(&outValue), sizeof(outValue));
            return file_.good();
        }

        /// @brief 文字列を書き込む
        /// @param str 
        /// @return 
        bool WriteString(const std::string& str)
        {
            uint32_t len = static_cast<uint32_t>(str.size());
            if (!WriteUint32(len)) return false;
            if (len > 0)
            {
                file_.write(str.data(), len);
            }
            return file_.good();
        }

        /// @brief 文字列を読み込む
        /// @param outStr 
        /// @return 
        bool ReadString(std::string& outStr)
        {
            uint32_t len = 0;
            if (!ReadUint32(len)) return false;
            outStr.resize(len);
            if (len > 0)
            {
                file_.read(outStr.data(), len);
            }
            return file_.good();
        }


	private:

		/// @brief ファイルストリーム
		std::fstream file_;
	};
}
