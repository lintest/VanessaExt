#include "stdafx.h"

#ifdef __linux__

#include <iconv.h>

std::wstring MB2WC(const std::string& source)
{
	std::string tocode = sizeof(wchar_t) == 4 ? "UTF-32" : "UTF-16";
	iconv_t cd = iconv_open(tocode.c_str(), "UTF-8");
	if (cd == (iconv_t)-1) return {};

	std::wstring result;
	result.resize(source.size() + 1);

	char* src = const_cast<char*>(source.data());
	wchar_t* trg = const_cast<wchar_t*>(result.data());

	size_t succeed = (size_t)-1;
	size_t f = source.size() * sizeof(char);
	size_t t = result.size() * sizeof(wchar_t);
	succeed = iconv(cd, (char**)&src, &f, (char**)&trg, &t);
	iconv_close(cd);
	if (succeed == (size_t)-1) return {};
	return result;
}

std::string WC2MB(const std::wstring& source)
{
	std::string fromcode = sizeof(wchar_t) == 4 ? "UTF-32" : "UTF-16";
	iconv_t cd = iconv_open("UTF-8", fromcode.c_str());
	if (cd == (iconv_t)-1) return {};

	std::string result;
	result.resize(source.size() * sizeof(wchar_t) + 1);

	wchar_t* src = const_cast<wchar_t*>(source.data());
	char* trg = const_cast<char*>(result.data());

	size_t succeed = (size_t)-1;
	size_t f = source.size() * sizeof(wchar_t);
	size_t t = result.size() * sizeof(char);
	succeed = iconv(cd, (char**)&src, &f, (char**)&trg, &t);
	iconv_close(cd);
	if (succeed == (size_t)-1) return {};
	return result;
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
	return (HWND)IntToPtr(paParams->lVal);
}

#endif//__linux__

int32_t VarToInt(tVariant* paParams)
{
	return paParams->lVal;
}

