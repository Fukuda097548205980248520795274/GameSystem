#pragma once
#include <string>

namespace Detail
{
	/// @brief string -> wstring
	/// @param str 
	/// @return 
	std::wstring ConvertString(const std::string& str);

	/// @brief wstring -> string
	/// @param str 
	/// @return 
	std::string ConvertString(const std::wstring& str);
}