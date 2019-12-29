// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef __STDAFX_H__
#define __STDAFX_H__

#include <locale>
#include <string>
#include <vector>
#include <types.h>

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"

#include "json.hpp"
using JSON = nlohmann::json;

#ifdef _WINDOWS
    #include <windows.h>
#else
    typedef unsigned long HWND;
#endif //_WINDOWS

#if defined(__linux__) || defined(__APPLE__)
	#define LINUX_OR_MACOS
#endif

std::wstring MB2WC(const std::string &source);

std::string WC2MB(const std::wstring &source);

long VarToInt(tVariant* paParams);

#ifdef _WINDOWS
HWND VarToHwnd(tVariant* paParams);
#endif //_WINDOWS

std::wstring WSTR(const JSON &json);

#endif //__STDAFX_H__
