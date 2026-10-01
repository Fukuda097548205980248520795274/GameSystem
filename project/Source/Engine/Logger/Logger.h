#pragma once
#include <string>
#include <fstream>

namespace Detail
{
	class Logger
	{
	public:

		/// @brief コンストラクタ
		Logger();

		/// @brief ロギング
		/// @param log 
		void Logging(const std::string& log);


	private:

		/// @brief 出力ファイルストリーム
		std::ofstream os;
	};


}