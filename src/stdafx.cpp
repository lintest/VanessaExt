#include "stdafx.h"

#ifdef _WINDOWS

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

#else //_WINDOWS

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

#endif //_WINDOWS

int32_t VarToInt(tVariant* paParams)
{
	return paParams->lVal;
}

bool AddInNative::Init(void* pConnection)
{
	m_iConnect = (IAddInDefBase*)pConnection;
	return m_iConnect != NULL;
}

long AddInNative::GetInfo()
{
	// Component should put supported component technology version 
	// This component supports 2.0 version
	return 2000;
}

void AddInNative::Done()
{
}

bool AddInNative::setMemManager(void* mem)
{
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != 0;
}

bool ADDIN_API AddInNative::AllocMemory(void** pMemory, unsigned long ulCountByte) const
{
	return m_iMemory ? m_iMemory->AllocMemory(pMemory, ulCountByte) : false;
}

void AddInNative::addError(const wchar_t* descriptor)
{
	if (m_iConnect) {
		WCHAR_T* err = 0;
		WCHAR_T* descr = 0;
		const wchar_t* source = L"VanessaExt";
		::convToShortWchar(&err, source);
		::convToShortWchar(&descr, descriptor);
		uint32_t wcode = ADDIN_E_VERY_IMPORTANT;
		m_iConnect->AddError(wcode, err, descr, -1);
		delete[] descr;
		delete[] err;
	}
}

std::wstring VarToStr(tVariant* paParams)
{
	wchar_t* str = nullptr;
	::convFromShortWchar(&str, paParams->pwstrVal);
	std::wstring result = str;
	delete[] str;
	return result;
}

uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
	uint32_t res = 0;
	WCHAR_T* tmpShort = (WCHAR_T*)Source;

	while (*tmpShort++)
		++res;

	return res;
}

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len)
{
	if (!len)
		len = (uint32_t)::wcslen(Source) + 1;

	if (!*Dest)
		*Dest = new WCHAR_T[len];

	WCHAR_T* tmpShort = *Dest;
	wchar_t* tmpWChar = (wchar_t*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(WCHAR_T));
#ifdef __linux__
	size_t succeed = (size_t)-1;
	size_t f = len * sizeof(wchar_t), t = len * sizeof(WCHAR_T);
	const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
	iconv_t cd = iconv_open("UTF-16LE", fromCode);
	if (cd != (iconv_t)-1)
	{
		succeed = iconv(cd, (char**)&tmpWChar, &f, (char**)&tmpShort, &t);
		iconv_close(cd);
		if (succeed != (size_t)-1)
			return (uint32_t)succeed;
	}
#endif //__linux__
	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
	{
		*tmpShort = (WCHAR_T)*tmpWChar;
	}

	return res;
}

uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
	if (!len)
		len = getLenShortWcharStr(Source) + 1;

	if (!*Dest)
		*Dest = new wchar_t[len];

	wchar_t* tmpWChar = *Dest;
	WCHAR_T* tmpShort = (WCHAR_T*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(wchar_t));
#ifdef __linux__
	size_t succeed = (size_t)-1;
	const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
	size_t f = len * sizeof(WCHAR_T), t = len * sizeof(wchar_t);
	iconv_t cd = iconv_open("UTF-32LE", fromCode);
	if (cd != (iconv_t)-1)
	{
		succeed = iconv(cd, (char**)&tmpShort, &f, (char**)&tmpWChar, &t);
		iconv_close(cd);
		if (succeed != (size_t)-1)
			return (uint32_t)succeed;
	}
#endif //__linux__
	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
	{
		*tmpWChar = (wchar_t)*tmpShort;
	}

	return res;
}
