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

		/// @brief 出力ファイルストリーム
		std::ofstream os;
	};


}