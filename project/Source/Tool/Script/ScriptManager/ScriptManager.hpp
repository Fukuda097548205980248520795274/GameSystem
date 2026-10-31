#pragma once
#include "Store/HandleData/HandleData.hpp"
#include <unordered_map>
#include <queue>
#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptstdstring/scriptstdstring.h>
#include <string>

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
		};

		// データテーブル
		std::vector<Slot> slots_;

		// 空きスロットのインデックスキュー
		std::queue<uint32_t> freeSlots_;

		// キャッシュ
		std::unordered_map<std::string, ScriptHandle> cache_;
	};
}