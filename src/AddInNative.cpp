#include "stdafx.h"

#include <minwindef.h>
#include <wtypes.h>
#include "AddInNative.h"
#include "convertor.h"
#include <memory>
#include <locale>
#include "WinCtrl.h"

static std::wstring param(tVariant* paParams, const long lSizeArray)
{
	std::wstring result;
	switch (TV_VT(paParams))
	{
	case VTYPE_PWSTR: {
		wchar_t* str = 0;
		::convFromShortWchar(&str, TV_WSTR(paParams));
		result = str;
		delete[] str;
	} break;
	default:
		break;
	}
	return result;
}

/////////////////////////////////////////////////////////////////////////////
// CAddInNative
//---------------------------------------------------------------------------//
CAddInNative::CAddInNative()
{
	m_iMemory = 0;
	m_iConnect = 0;
}
//---------------------------------------------------------------------------//
CAddInNative::~CAddInNative()
{
}
//---------------------------------------------------------------------------//
bool CAddInNative::Init(void* pConnection)
{
	m_iConnect = (IAddInDefBase*)pConnection;
	return m_iConnect != NULL;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetInfo()
{
	// Component should put supported component technology version 
	// This component supports 2.0 version
	return 2000;
}
//---------------------------------------------------------------------------//
void CAddInNative::Done()
{
}
/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{
	return W(L"WindowsControl", wsExtensionName);
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNProps()
{
	return ePropLast;
}

//---------------------------------------------------------------------------//
long CAddInNative::FindName(const ADDIN_NAMES names, long size, const WCHAR_T* name)
{
	for (long i = 0; i < size; i++) {
		for (long j = 0; j < ALIAS_COUNT; j++) {
			if (wcsicmp(names[i][j], name) == 0) return i;
		}
	}
	return -1;
}

//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetName(const ADDIN_NAMES names, long size, long lPropNum, long lPropAlias)
{
	if (lPropNum >= size) return NULL;
	if (lPropAlias >= ALIAS_COUNT) return NULL;
	return W((wchar_t*)names[lPropNum][lPropAlias]);
}

//---------------------------------------------------------------------------//
long CAddInNative::FindProp(const WCHAR_T* wsPropName)
{
	return FindName(g_PropNames, ePropLast, wsPropName);
}

//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::W(const wchar_t* str) const
{
	WCHAR_T* res = NULL;
	W(str, &res);
	return res;
}

//---------------------------------------------------------------------------//
BOOL CAddInNative::W(const wchar_t* str, WCHAR_T** res) const
{
	if (m_iMemory && str) {
		size_t size = wcslen(str) + 1;
		if (m_iMemory->AllocMemory((void**)res, size * sizeof(WCHAR_T))) {
			::convToShortWchar(res, str, size);
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------------//
BOOL CAddInNative::W(const wchar_t* str, tVariant* res) const
{
	TV_VT(res) = VTYPE_PWSTR;
	res->pwstrVal = NULL;
	res->wstrLen = 0;
	if (m_iMemory && str) {
		size_t size = wcslen(str) + 1;
		if (m_iMemory->AllocMemory((void**)&res->pwstrVal, size * sizeof(WCHAR_T))) {
			::convToShortWchar((WCHAR_T**)&res->pwstrVal, str, size);
			res->wstrLen = size;
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------------//
BOOL CAddInNative::W(const DWORD& val, tVariant* res) const
{
	TV_VT(res) = VTYPE_UI4;
	res->uintVal = (uint32_t)val;
	return true;
}

//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias)
{
	return GetName(g_PropNames, ePropLast, lPropNum, lPropAlias);
}

//---------------------------------------------------------------------------//
bool CAddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum) {
	case eCurrentWindow:
		return W((DWORD)WindowsControl::CurrentWindow(), pvarPropVal);
	case eActiveWindow:
		return W((DWORD)WindowsControl::ActiveWindow(), pvarPropVal);
	case eProcessId:
		return W(WindowsControl::ProcessId(), pvarPropVal);
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool CAddInNative::SetPropVal(const long lPropNum, tVariant* varPropVal)
{
	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropReadable(const long lPropNum)
{
	return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropWritable(const long lPropNum)
{
	return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNMethods()
{
	return eMethLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindMethod(const WCHAR_T* wsMethodName)
{
	return FindName(g_MethodNames, eMethLast, wsMethodName);
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{
	return GetName(g_MethodNames, eMethLast, lMethodNum, lMethodAlias);
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNParams(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eSetWindowSize:
		return 3;
	case eSetWindowPos:
		return 3;
	case eActivateWindow:
		return 1;
	case eEnableResizing:
		return 2;
	case eTakeScreenshot:
		return 1;
	case eCaptureWindow:
		return 1;
	default:
		return 0;
	}
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue)
{
	TV_VT(pvarParamDefValue) = VTYPE_EMPTY;
	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::HasRetVal(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eGetWindowList:
	case eSetWindowSize:
	case eSetWindowPos:
	case eActivateWindow:
	case eTakeScreenshot:
	case eCaptureWindow:
		return true;
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case eGetWindowList:
		WindowsControl::GetWindowList();
		return true;
	case eSetWindowSize:
		return WindowsControl::SetWindowSize(paParams, lSizeArray);
	case eSetWindowPos:
		return WindowsControl::SetWindowPos(paParams, lSizeArray);
	case eActivateWindow:
		return WindowsControl::ActivateWindow(paParams, lSizeArray);
	case eEnableResizing:
		return WindowsControl::EnableResizing(paParams, lSizeArray);
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool CAddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum) {
	case eGetWindowList:
		return W(WindowsControl::GetWindowList(), pvarRetValue);
	case eSetWindowSize:
		return W(WindowsControl::SetWindowSize(paParams, lSizeArray), pvarRetValue);
	case eSetWindowPos:
		return W(WindowsControl::SetWindowPos(paParams, lSizeArray), pvarRetValue);
	case eActivateWindow:
		return W(WindowsControl::ActivateWindow(paParams, lSizeArray), pvarRetValue);
	case eEnableResizing:
		return W(WindowsControl::EnableResizing(paParams, lSizeArray), pvarRetValue);
	case eTakeScreenshot:
		return WindowsControl(m_iMemory).CaptureScreen(pvarRetValue, paParams, lSizeArray);
	case eCaptureWindow:
		return WindowsControl(m_iMemory).CaptureWindow(pvarRetValue, paParams, lSizeArray);
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
void CAddInNative::SetLocale(const WCHAR_T* loc)
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
bool CAddInNative::setMemManager(void* mem)
{
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != 0;
}
//---------------------------------------------------------------------------//
void CAddInNative::addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code)
{
	if (m_iConnect)
	{
		WCHAR_T* err = 0;
		WCHAR_T* descr = 0;

		::convToShortWchar(&err, source);
		::convToShortWchar(&descr, descriptor);

		m_iConnect->AddError(wcode, err, descr, code);
		delete[] err;
		delete[] descr;
	}
}
//---------------------------------------------------------------------------//

