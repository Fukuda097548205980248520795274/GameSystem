#include "Logger.h"
#include <Windows.h>
#include <chrono>
#include <cassert>
#include <filesystem>

/// @brief コンストラクタ
Detail::Logger::Logger()
{
	// logsディレクトリがなければ作る
	std::error_code ec;
	if (!std::filesystem::create_directories("./logs", ec) && ec)
	{
		// ディレクトリ作成に失敗した場合のエラーハンドリング
		assert(false);
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
		OutputDebugStringA("ログファイルを生成できませんでした \n");
		assert(false);
	}

	// ログファイル作成のログを出力
	Logging(LogLevel::Info, "ログファイル生成");
}

/// @brief ログを出力する
/// @param level ログレベル
/// @param log ログメッセージ
void Detail::Logger::Logging(LogLevel level, const std::string& log)
{
	// 現在時刻を取得
	auto now = std::chrono::system_clock::now();
	auto localTime = std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::floor<std::chrono::seconds>(now) };

	// ログレベルに応じた文字列を決定
	const char* levelStr = (level == LogLevel::Error) ? "[ERROR]" :
		(level == LogLevel::Warning) ? "[WARN ]" : "[INFO ]";

	// フォーマットされたログメッセージを作成
	std::string formattedLog = std::format("[{:%Y-%m-%d %H:%M:%S}] {} {}\n", localTime, levelStr, log);

	// 標準出力に出力
	os << formattedLog;
	if (level == LogLevel::Error || level == LogLevel::Warning)
	{
		// エラーや警告の場合は即座にフラッシュしてログを出力
		os.flush();
	}

	// デバッグ出力にも出力
	OutputDebugStringA(formattedLog.c_str());
}