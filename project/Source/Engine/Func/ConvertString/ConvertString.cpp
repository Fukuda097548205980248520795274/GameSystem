#include "ConvertString.h"
#include <Windows.h>

/// @brief string -> wstring
/// @param str 
/// @return 
std::wstring Detail::ConvertString(const std::string& str)
{
	// 空文字列の場合は空のwstringを返す
    if (str.empty())return std::wstring();

	// 文字列の長さを取得する
    auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
    if (sizeNeeded == 0)return std::wstring();

	// wstringを作成する
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
    return result;
}

/// @brief wstring -> string
/// @param str 
/// @return 
std::string Detail::ConvertString(const std::wstring& str)
{
	// 空文字列の場合は空のstringを返す
    if (str.empty())return std::string();

	// 文字列の長さを取得する
    auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
    if (sizeNeeded == 0) return std::string();

	// stringを作成する
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
    return result;
}