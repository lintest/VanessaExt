// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef __STDAFX_H__
#define __STDAFX_H__

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

std::u16string upper(std::u16string& str);
std::u16string lower(std::u16string& str);
std::wstring upper(std::wstring& str);
std::wstring lower(std::wstring& str);

#include "json.hpp"
using JSON = nlohmann::json;

constexpr auto PROCESS_FINISHED = u"PROCESS_FINISHED";

#endif //__STDAFX_H__
