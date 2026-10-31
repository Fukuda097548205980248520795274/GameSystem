#pragma once
#include "Store/HandleData/HandleData.hpp"
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
        /// @param scriptPath 
        /// @return 
        ScriptHandle BuildScriptModule(const std::string& moduleName, const std::string& scriptPath);

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

		/// @brief 変更されたスクリプトをリロードする
        void ReloadModifiedScripts();


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

			// モジュール名
			std::string moduleName = "";

			// スクリプトファイルの最終更新時刻
            std::filesystem::file_time_type lastWriteTime;
		};

		// データテーブル
		std::vector<Slot> slots_;

		// 空きスロットのインデックスキュー
		std::queue<uint32_t> freeSlots_;

		// キャッシュ
		std::unordered_map<std::string, ScriptHandle> cache_;


	private:

		// 中間データ用ディレクトリ
		const std::string kCookedDir = "./Cooked/Script/";
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
            return 0;
        }

		/// @brief 書き込み
        /// @param ptr 
        /// @param size 
        /// @return 
        int Write(const void* ptr, asUINT size) override
        {
            if (!file_.is_open()) return -1;
            file_.write(static_cast<const char*>(ptr), size);
            return 0;
        }

		/// @brief ファイルが開いているかどうかを確認する
        /// @return 
        bool IsOpen() const { return file_.is_open(); }

    private:

		/// @brief ファイルストリーム
        std::fstream file_;
    };
}