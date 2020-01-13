#include "stdafx.h"

#ifdef _WINDOWS
#pragma setlocale("ru-RU" )
#else //_WINDOWS
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <iconv.h>
#include <sys/time.h>
#endif //_WINDOWS

#include <stdio.h>
#include <wchar.h>
#include <string>
#include "AddInBase.h"
#include "WindowsControl.h"
#include "ClipboardControl.h"

static const wchar_t g_kClassNames[] = L"WindowsControl|ClipboardControl";

uint32_t getLenShortWcharStr(const WCHAR_T* Source);
static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;
static WcharWrapper s_names(g_kClassNames);

class WSTR {
private:
	const wchar_t* m_str = 0;
public:
	WSTR(const wchar_t* str) {
		#ifdef _WINDOWS
			m_str = str;
		#else
			::convFromShortWchar(&m_str, str);
		#endif
	}
	~WSTR() { 
		#ifndef _WINDOWS
			delete[] m_str;
		#endif
	}
	bool operator ==(const WCHAR_T* str) const {
		#ifdef _WINDOWS
			return wcsicmp(m_str, str) == 0;
		#else
			return wcscasecmp(m_str, str) == 0);
		#endif
	}
};

//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface)
{
	if (!*pInterface) {
		*pInterface = 0;
		WSTR wstr(wsName);
		if (wstr == L"WindowsControl") *pInterface = new WindowsControl;
		if (wstr == L"ClipboardControl") *pInterface = new ClipboardControl;
		return (long)*pInterface;
	}
	return 0;
}
//---------------------------------------------------------------------------//
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
	g_capabilities = capabilities;
	return eAppCapabilitiesLast;
}
//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase** pIntf)
{
	if (!*pIntf)
		return -1;

	delete* pIntf;
	*pIntf = 0;
	return 0;
}
//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames()
{
	return s_names;
}
//---------------------------------------------------------------------------//
// WindowManager
//---------------------------------------------------------------------------//
AddInBase::AddInBase()
{
	m_iMemory = 0;
	m_iConnect = 0;
}
//---------------------------------------------------------------------------//
AddInBase::~AddInBase()
{
}
//---------------------------------------------------------------------------//
bool AddInBase::Init(void* pConnection)
{
	m_iConnect = (IAddInDefBase*)pConnection;
	return m_iConnect != NULL;
}
//---------------------------------------------------------------------------//
long AddInBase::GetInfo()
{
	// Component should put supported component technology version 
	// This component supports 2.0 version
	return 2000;
}
//---------------------------------------------------------------------------//
void AddInBase::Done()
{
}
//---------------------------------------------------------------------------//
long AddInBase::FindName(const std::vector<Alias>& names, const WCHAR_T* name)
{
	WSTR wstr(name);
	for (Alias alias : names) {
		for (long i = 0; i < m_AliasCount; i++) {
			if (wstr == alias.Name(i)) return alias.id;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------//
const WCHAR_T* AddInBase::GetName(const std::vector<Alias>& names, long lPropNum, long lPropAlias)
{
	if (lPropNum >= names.size()) return NULL;
	if (lPropAlias >= m_AliasCount) return NULL;
	for (Alias alias : names) {
		if (alias.id == lPropNum) {
			return W((wchar_t*)alias.Name(lPropAlias));
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------//
long AddInBase::GetNParams(const std::vector<Alias>& names, const long lMethodNum)
{
	for (Alias alias : names) {
		if (alias.id == lMethodNum) return alias.np;
	}
	return 0;
}
//---------------------------------------------------------------------------//
bool AddInBase::HasRetVal(const std::vector<Alias>& names, const long lMethodNum)
{
	for (Alias alias : names) {
		if (alias.id == lMethodNum) return alias.fn;
	}
	return 0;
}
//---------------------------------------------------------------------------//
AddInBase::VarinantHelper& AddInBase::VarinantHelper::operator<<(const wchar_t* str)
{
	TV_VT(pvar) = VTYPE_PWSTR;
	pvar->pwstrVal = NULL;
	if (mm && str && wcslen(str)) {
		unsigned long size = (unsigned long)wcslen(str) + 1;
		if (mm->AllocMemory((void**)&pvar->pwstrVal, size * sizeof(WCHAR_T))) {
			::convToShortWchar((WCHAR_T**)&pvar->pwstrVal, str, size);
			pvar->wstrLen = size - 1;
		}
	}
	return *this;
}

AddInBase::VarinantHelper& AddInBase::VarinantHelper::operator<<(const std::wstring& str)
{
	return operator<<(str.c_str());
}

AddInBase::VarinantHelper& AddInBase::VarinantHelper::operator<<(int64_t value)
{
	TV_VT(pvar) = VTYPE_I4;
	TV_I8(pvar) = value;
	return *this;
}

AddInBase::VarinantHelper& AddInBase::VarinantHelper::operator<<(int32_t value)
{
	TV_VT(pvar) = VTYPE_I4;
	TV_I4(pvar) = value;
	return *this;
}

const WCHAR_T* AddInBase::W(const wchar_t* str) const
{
	WCHAR_T* res = NULL;
	if (m_iMemory && str && wcslen(str)) {
		unsigned long size = (unsigned long)wcslen(str) + 1;
		if (m_iMemory->AllocMemory((void**)res, size * sizeof(WCHAR_T))) {
			::convToShortWchar(&res, str, size);
		}
	}
	return res;
}

/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool AddInBase::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{
	const wchar_t* wsExtension = ExtensionName();
	unsigned long iActualSize = (unsigned long)::wcslen(wsExtension) + 1;
	WCHAR_T* dest = 0;

	if (m_iMemory)
	{
		if (m_iMemory->AllocMemory((void**)wsExtensionName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(wsExtensionName, wsExtension, iActualSize);
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------//
long AddInBase::GetNProps()
{
	return PropList().size();
}
//---------------------------------------------------------------------------//
long AddInBase::FindProp(const WCHAR_T* wsPropName)
{
	return FindName(PropList(), wsPropName);
}
//---------------------------------------------------------------------------//
const WCHAR_T* AddInBase::GetPropName(long lPropNum, long lPropAlias)
{
	return GetName(PropList(), lPropNum, lPropAlias);
}
//---------------------------------------------------------------------------//
bool AddInBase::IsPropReadable(const long lPropNum)
{
	return true;
}
//---------------------------------------------------------------------------//
bool AddInBase::IsPropWritable(const long lPropNum)
{
	return HasRetVal(PropList(), lPropNum);
}
//---------------------------------------------------------------------------//
long AddInBase::GetNMethods()
{
	return MethList().size();
}
//---------------------------------------------------------------------------//
long AddInBase::FindMethod(const WCHAR_T* wsMethodName)
{
	return FindName(MethList(), wsMethodName);
}
//---------------------------------------------------------------------------//
const WCHAR_T* AddInBase::GetMethodName(const long lMethodNum, const long lMethodAlias)
{
	return GetName(MethList(), lMethodNum, lMethodAlias);
}
//---------------------------------------------------------------------------//
long AddInBase::GetNParams(const long lMethodNum)
{
	return GetNParams(MethList(), lMethodNum);
}
//---------------------------------------------------------------------------//
bool AddInBase::GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue)
{
	TV_VT(pvarParamDefValue) = VTYPE_EMPTY;
	return false;
}
//---------------------------------------------------------------------------//
bool AddInBase::HasRetVal(const long lMethodNum)
{
	return HasRetVal(MethList(), lMethodNum);
}
//---------------------------------------------------------------------------//
void AddInBase::SetLocale(const WCHAR_T* loc)
{
#if !defined( __linux__ ) && !defined(__APPLE__)
	_wsetlocale(LC_ALL, loc);
#else
	//We convert in char* char_locale
	//also we establish locale
	//setlocale(LC_ALL, char_locale);
#endif
}
/////////////////////////////////////////////////////////////////////////////
// LocaleBase
//---------------------------------------------------------------------------//
bool AddInBase::setMemManager(void* mem)
{
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != 0;
}
//---------------------------------------------------------------------------//
void AddInBase::addError(uint32_t wcode, const wchar_t* source,
	const wchar_t* descriptor, long code)
{
	if (m_iConnect)
	{
		WCHAR_T* err = 0;
		WCHAR_T* descr = 0;

		::convToShortWchar(&err, source);
		::convToShortWchar(&descr, descriptor);

		m_iConnect->AddError(wcode, err, descr, code);
		delete[] err;
		delete[] err;
		delete[] descr;
	}
}
//---------------------------------------------------------------------------//
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
//---------------------------------------------------------------------------//
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
//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
	uint32_t res = 0;
	WCHAR_T* tmpShort = (WCHAR_T*)Source;

	while (*tmpShort++)
		++res;

	return res;
}
//---------------------------------------------------------------------------//

#ifdef LINUX_OR_MACOS
WcharWrapper::WcharWrapper(const WCHAR_T* str) : m_str_WCHAR(NULL),
m_str_wchar(NULL)
{
	if (str)
	{
		int len = getLenShortWcharStr(str);
		m_str_WCHAR = new WCHAR_T[len + 1];
		memset(m_str_WCHAR, 0, sizeof(WCHAR_T) * (len + 1));
		memcpy(m_str_WCHAR, str, sizeof(WCHAR_T) * len);
		::convFromShortWchar(&m_str_wchar, m_str_WCHAR);
	}
}
#endif
//---------------------------------------------------------------------------//
WcharWrapper::WcharWrapper(const wchar_t* str) :
#ifdef LINUX_OR_MACOS
	m_str_WCHAR(NULL),
#endif 
	m_str_wchar(NULL)
{
	if (str)
	{
		size_t len = wcslen(str);
		m_str_wchar = new wchar_t[len + 1];
		memset(m_str_wchar, 0, sizeof(wchar_t) * (len + 1));
		memcpy(m_str_wchar, str, sizeof(wchar_t) * len);
#ifdef LINUX_OR_MACOS
		::convToShortWchar(&m_str_WCHAR, m_str_wchar);
#endif
	}

}
//---------------------------------------------------------------------------//
WcharWrapper::~WcharWrapper()
{
#ifdef LINUX_OR_MACOS
	if (m_str_WCHAR)
	{
		delete[] m_str_WCHAR;
		m_str_WCHAR = NULL;
	}
#endif
	if (m_str_wchar)
	{
		delete[] m_str_wchar;
		m_str_wchar = NULL;
	}
}
//---------------------------------------------------------------------------//
