#pragma once
#include <../include/types.h>
#include <string>

#ifdef _WINDOWS
extern HMODULE hModule;
#endif //_WINDOWS

std::wstring MB2WC(const std::string& source);
std::string WC2MB(const std::wstring& source);
std::u16string MB2WCHAR(std::string_view src);
std::string WCHAR2MB(std::basic_string_view<WCHAR_T> src);
std::wstring WCHAR2WC(std::basic_string_view<WCHAR_T> src);

std::u16string upper(const std::u16string& str);
std::u16string lower(const std::u16string& str);
std::wstring upper(const std::wstring& str);
std::wstring lower(const std::wstring& str);

#include "json.hpp"
using JSON = nlohmann::json;

constexpr auto PROCESS_FINISHED = u"PROCESS_FINISHED";
