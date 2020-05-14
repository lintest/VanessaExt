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
#include "ProcessControl.h"
#include "ClipboardControl.h"

static const wchar_t g_kClassNames[] = L"WindowsControl|ClipboardControl|ProcessControl";

uint32_t getLenShortWcharStr(const WCHAR_T* Source);
static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;
static WcharWrapper s_names(g_kClassNames);

#ifdef _WINDOWS

class WSTR {
private:
	const wchar_t* m_str = 0;
public:
	WSTR(const WCHAR_T* str) :m_str(str) {}
	bool operator ==(const wchar_t* str) const {
		return wcsicmp(m_str, str) == 0;
	}
};

#else //_WINDOWS

class WSTR {
private:
	wchar_t* m_str = 0;
public:
	WSTR(const WCHAR_T* str) {
		::convFromShortWchar(&m_str, str);
	}
	~WSTR() {
		delete[] m_str;
	}
	bool operator ==(const wchar_t* str) const {
		return wcscasecmp(m_str, str) == 0;
	}
};

#endif //_WINDOWS

//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface)
{
	if (!*pInterface) {
		*pInterface = 0;
		WSTR wstr(wsName);
		if (wstr == L"WindowsControl") *pInterface = new WindowsControl;
		if (wstr == L"ProcessControl") *pInterface = new ProcessControl;
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
// AddInBase
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
	if (lPropAlias >= m_AliasCount) return NULL;
	if (lPropNum >= (long)names.size()) return NULL;
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
	if (addin && str && wcslen(str)) {
		unsigned long size = (unsigned long)wcslen(str) + 1;
		if (addin->AllocMemory((void**)&pvar->pwstrVal, size * sizeof(WCHAR_T))) {
			::convToShortWchar((WCHAR_T**)&pvar->pwstrVal, str, size);
			pvar->wstrLen = size - 1;
			while (pvar->wstrLen && pvar->pwstrVal[pvar->wstrLen - 1] == 0) pvar->wstrLen--;
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
	if (str && wcslen(str)) {
		unsigned long size = (unsigned long)wcslen(str) + 1;
		if (AllocMemory((void**)res, size * sizeof(WCHAR_T))) {
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
	if (AllocMemory((void**)wsExtensionName, iActualSize * sizeof(WCHAR_T))) {
		::convToShortWchar(wsExtensionName, wsExtension, iActualSize);
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------//
long AddInBase::GetNProps()
{
	return (long)PropList().size();
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
	return (long)MethList().size();
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
