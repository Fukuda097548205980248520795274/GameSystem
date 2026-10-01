#include "Logger.h"
#include <Windows.h>
#include <chrono>
#include <cassert>

/// @brief コンストラクタ
Detail::Logger::Logger()
{
	// ディレクトリを掘る
	if (!CreateDirectory(L"./logs", nullptr))
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			assert(false);
		}
	}

	// 現在時刻（UTC時刻）を取得
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	// ログファイルをコンマを使わず秒にする
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> nowSeconds =
		std::chrono::time_point_cast<std::chrono::seconds>(now);

	// 日本時間（PCの設定時間）に変換
	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };

	// formatを使って年月日_時分秒の文字列に変換
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);

	// ファイル名は時刻を使う
	std::string logFilePath = std::string("./logs/") + dateString + ".log";

	// ファイルを作って書き込み準備
	os.open(logFilePath);
	if (!os.is_open())
	{
		OutputDebugStringA("Failed to open log file\n");
		assert(false);
	}

	Logging("Create Log File \n");
}

/// @brief ロギング
/// @param log 
void Detail::Logger::Logging(const std::string& log)
{
	os << log << '\n';
	os.flush();

	OutputDebugStringA(log.c_str());
}