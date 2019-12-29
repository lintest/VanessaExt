#include "stdafx.h"

#ifdef __linux__

#include <iconv.h>

std::wstring MB2WC(const std::string &source)
{
    std::string tocode = sizeof(wchar_t) == 4 ? "UTF-32" : "UTF-16";
    iconv_t cd = iconv_open(tocode.c_str(), "UTF-8");
    if (cd == (iconv_t)-1) return {};

    size_t ilen = source.size() * sizeof(char);
    size_t olen = (source.size() + 1) * sizeof(wchar_t);
    char* ip = const_cast<char*>(source.data());
    char* outbuf = (char*)malloc(olen);
    if (outbuf == NULL) { iconv_close(cd); return {}; } 

    char *op = outbuf;
    std::wstring result;
    size_t icount = ilen;
    size_t ocount = olen;
    size_t succeed = iconv(cd, (char**)&ip, &icount, (char**)&op, &ocount); 
    if (succeed != (size_t)-1) {
        outbuf[olen - ocount] = '\0';
        result = (wchar_t*)outbuf;
    }
    free(outbuf); 
    iconv_close(cd);
    return result;
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

std::string iconv_recode(std::string from, std::string to, std::string text)
{
    iconv_t cnv = iconv_open(to.c_str(), from.c_str());
    if (cnv == (iconv_t)-1) {
        iconv_close(cnv);
        return "";
    }

    char *outbuf;
    if ((outbuf = (char *)malloc(text.length()*2 + 1)) == NULL) {
        iconv_close(cnv);
        return "";
    }

    char *ip = (char*) text.c_str(), *op = outbuf;
    size_t icount = text.length(), ocount = text.length()*2;

    if (iconv(cnv, &ip, &icount, &op, &ocount) != (size_t) - 1) {
        outbuf[text.length()*2 - ocount] = '\0';
        text = outbuf;
    } else {
        text = "";
    }

    free(outbuf);
    iconv_close(cnv);
    return text;
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

HWND VarToHwnd(tVariant* paParams)
{
    return (HWND)IntToPtr(paParams->intVal);
}

#endif//__linux__

long VarToInt(tVariant* paParams)
{
    return paParams->intVal;
}

std::wstring WSTR(const JSON &json) {
    if (json.empty()) return {};
    return MB2WC(json.dump());
}
