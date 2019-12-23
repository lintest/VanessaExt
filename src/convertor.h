#ifndef __CONVERTOR_H__
#define __CONVERTOR_H__

#include "types.h"
#include <string>

#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);

std::string WC2MB(const std::wstring& wstr, DWORD locale = CP_UTF8);
std::wstring MB2WC(const std::string& str, DWORD locale = CP_UTF8);

#ifndef __linux__
HWND VarToHwnd(tVariant* paParams);
#endif //__linux__

int VarToInt(tVariant* paParams);

#endif // __CONVERTOR_H__
