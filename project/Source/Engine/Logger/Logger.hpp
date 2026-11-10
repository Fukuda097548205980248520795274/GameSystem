#pragma once
#include <string>
#include <fstream>

// @brief ログレベル
enum class LogLevel
{
	Info,
	Warning,
	Error
};

namespace Detail
{
	/// @brief ロガー
	class Logger
	{
	public:

		/// @brief コンストラクタ
		Logger();

		/// @brief ログを出力する
		/// @param level
		/// @param log 
		void Logging(LogLevel level, const std::string& log);


	private:

        /// @brief デバッグ出力（UTF-8文字列）を出力する
        /// @param utf8Str 
        void DebugOutputUTF8(const std::string& utf8Str);


		/// @brief 出力ファイルストリーム
		std::ofstream os;
	};


}
