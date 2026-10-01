#pragma once
#include <Windows.h>
#include <DbgHelp.h>
#include <strsafe.h>

namespace Detail
{
	/// @brief 例外が発生した際に、Dumpファイルを出力する
	/// @param exception 
	/// @return 
	static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
	{
		// 時刻を取得し、名前に使用する
		SYSTEMTIME time;
		GetLocalTime(&time);
		wchar_t filePath[MAX_PATH] = { 0 };
		CreateDirectory(L"./dumps", nullptr);
		StringCchPrintfW(filePath, MAX_PATH, L"./dumps/%04d-%02d%02d-%02d%02d.dmp",
			time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
		HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

		// ProcessIDとクラッシュ（例外）の発生したthreadIDを取得
		DWORD processID = GetCurrentProcessId();
		DWORD threadID = GetCurrentThreadId();

		// 設定情報を入力
		MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
		minidumpInformation.ThreadId = threadID;
		minidumpInformation.ExceptionPointers = exception;
		minidumpInformation.ClientPointers = TRUE;

		// Dumpを出力
		MiniDumpWriteDump(GetCurrentProcess(), processID, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

		return EXCEPTION_EXECUTE_HANDLER;
	}
}