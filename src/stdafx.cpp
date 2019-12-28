#include "stdafx.h"

#ifdef __linux__

#include <iconv.h>

std::wstring MB2WC(const std::string &source)
{
    size_t len = source.size();
    std::wstring result;
    result.resize(len + 1);

    char* src = source.data();
    wchar_t* trg = result.data();

    size_t succeed = (size_t)-1;
    size_t f = len * sizeof(char), t = len * sizeof(wchar_t);
    iconv_t cd = iconv_open("UTF-32", "UTF-8");
    if (cd != (iconv_t)-1) {
        succeed = iconv(cd, (char**)&src, &f, (char**)&trg, &t);
        iconv_close(cd);
        if(succeed != (size_t)-1)
            return result;
    }
    return {};
}

std::string WC2MB(const std::wstring &source)
{
    size_t len = source.size();
    std::string result;
    result.resize(len * sizeof(wchar_t) + 1);

    wchar_t* src = const_cast<wchar_t*>(source.data());
    char* trg = const_cast<char*>(result.data());

    size_t succeed = (size_t)-1;
    size_t f = len * sizeof(wchar_t), t = f;
    iconv_t cd = iconv_open("UTF-8", "UTF-32");
    if (cd != (iconv_t)-1) {
        succeed = iconv(cd, (char**)&src, &f, (char**)&trg, &t);
        iconv_close(cd);
        if(succeed != (size_t)-1)
            return result;
    }
    return {};
}

#else//__linux__

std::string WC2MB(const std::wstring& wstr)
{
    DWORD locale = CP_UTF8;
    if (wstr.empty()) return {};
    const int sz = WideCharToMultiByte(locale, 0, &wstr[0], (int)wstr.size(), 0, 0, 0, 0);
    std::string res(sz, 0);
    WideCharToMultiByte(locale, 0, &wstr[0], (int)wstr.size(), &res[0], sz, 0, 0);
    return res;
}

std::wstring MB2WC(const std::string& str)
{
    DWORD locale = CP_UTF8;
    if (str.empty()) return {};
    const int sz = MultiByteToWideChar(locale, 0, &str[0], (int)str.size(), 0, 0);
    std::wstring res(sz, 0);
    MultiByteToWideChar(locale, 0, &str[0], (int)str.size(), &res[0], sz);
    return res;
}

long VarToInt(tVariant* paParams)
{
    return paParams->intVal;
}

HWND VarToHwnd(tVariant* paParams)
{
    return (HWND)IntToPtr(paParams->intVal);
}

#endif//__linux__
