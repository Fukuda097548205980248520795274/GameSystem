#pragma once
#include <Windows.h>

namespace Detail
{
    /// @brief クラッシュハンドラーの初期化
    void InitializeCrashHandler();

    /// @brief 例外が発生した際にDumpファイルを出力する
    /// @param exception 
    /// @return 
    LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);
}