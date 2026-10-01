#include "CrashHandler.h"
#include <strsafe.h>
#include <DbgHelp.h>

/// @brief 例外が発生した際にDumpファイルを出力する
/// @param exception 
/// @return 
LONG WINAPI Detail::ExportDump(EXCEPTION_POINTERS* exception)
{
    // dumps ディレクトリを作成
    CreateDirectory(L"./dumps", nullptr);

    // 時刻を取得（秒・ミリ秒まで含める）
    SYSTEMTIME time;
    GetLocalTime(&time);

	// ファイル名を作成
    wchar_t filePath[MAX_PATH] = { 0 };
    StringCchPrintfW(
        filePath, MAX_PATH,
        L"./dumps/%04d%02d%02d_%02d%02d%02d_%d.dmp",
        time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute, time.wSecond,
        GetCurrentProcessId()
    );

    // ファイル生成
    HANDLE dumpFileHandle = CreateFileW(
        filePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    // ハンドルが正常に取れた場合のみ書き込み
    if (dumpFileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD processID = GetCurrentProcessId();
        DWORD threadID = GetCurrentThreadId();

        MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
        minidumpInformation.ThreadId = threadID;
        minidumpInformation.ExceptionPointers = exception;
        minidumpInformation.ClientPointers = FALSE; // 自プロセスの場合は FALSE

        // 情報量を増やしてスタック上の参照メモリもダンプに含める
        MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
            MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory
            );

        MiniDumpWriteDump(
            GetCurrentProcess(),
            processID,
            dumpFileHandle,
            dumpType,
            &minidumpInformation,
            nullptr,
            nullptr
        );

        // リソースの解放
        CloseHandle(dumpFileHandle);
    }

    // ユーザーへエラーを通知
    MessageBoxW(
        nullptr,
        L"予期せぬエラーが発生したためアプリケーションを終了します。\n./dumps フォルダにエラーログを出力しました。",
        L"エラー",
        MB_OK | MB_ICONERROR
    );

    return EXCEPTION_EXECUTE_HANDLER;
}

/// @brief クラッシュハンドラーの初期化
void Detail::InitializeCrashHandler()
{
    SetUnhandledExceptionFilter(ExportDump);
}