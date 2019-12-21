#include "stdafx.h"

#include <minwindef.h>
#include <wtypes.h>
#include "AddInNative.h"
#include "convertor.h"
#include <memory>
#include <locale>

#include "WinCtrl.h"
#include "ProcMngr.h"

const std::vector<CAddInNative::Alias> CAddInNative::m_PropList = {
	Alias(eCurrentWindow , 0, true, L"CurrentWindow"   , L"ТекущееОкно"),
	Alias(eActiveWindow  , 0, true, L"ActiveWindow"    , L"АктивноеОкно"),
	Alias(eProcessId     , 0, true, L"ProcessId"       , L"ИдентификаторПроцесса"),
};

const std::vector<CAddInNative::Alias> CAddInNative::m_MethList = {
	Alias(eFindTestClient , 2, true , L"FindTestClient" , L"НайтиКлиентТестирования"),
	Alias(eGetProcessList , 1, true , L"GetProcessList" , L"ПолучитьСписокПроцессов"),
	Alias(eGetProcessInfo , 1, true , L"GetProcessInfo" , L"ПолучитьСвойстваПроцесса"),
	Alias(eFindProcess    , 1, true , L"FindProcess"    , L"НайтиПроцесс"),
	Alias(eGetWindowList  , 1, true , L"GetWindowList"  , L"ПолучитьСписокОкон"),
	Alias(eGetWindowInfo  , 1, true , L"GetWindowInfo"  , L"ПолучитьСвойстваОкна"),
	Alias(eGetWindowState , 1, true , L"GetWindowState" , L"ПолучитьСтатусОкна"),
	Alias(eGetWindowText  , 1, true , L"GetWindowText"  , L"ПолучитьЗаголовок"),
	Alias(eTakeScreenshot , 1, true , L"TakeScreenshot" , L"ПолучитьСнимокЭкрана"),
	Alias(eCaptureWindow  , 1, true , L"CaptureWindow"  , L"ПолучитьСнимокОкна"),
	Alias(eEnableResizing , 2, false, L"EnableResizing" , L"РазрешитьИзменятьРазмер"),
	Alias(eSetWindowPos   , 3, false, L"SetWindowPos"   , L"УстановитьПозициюОкна"),
	Alias(eSetWindowSize  , 3, false, L"SetWindowSize"  , L"УстановитьРазмерОкна"),
	Alias(eSetWindowState , 3, false, L"SetWindowState" , L"УстановитьСтатусОкна"),
	Alias(eSetWindowText  , 2, false, L"SetWindowText"  , L"УстановитьЗаголовок"),
	Alias(eActivateWindow , 1, false, L"ActivateWindow" , L"АктивироватьОкно"),
	Alias(eMaximizeWindow , 1, false, L"MaximizeWindow" , L"РаспахнутьОкно"),
	Alias(eMinimizeWindow , 1, false, L"MinimizeWindow" , L"СвернутьОкно"),
	Alias(eRestoreWindow  , 1, false, L"RestoreWindow"  , L"РазвернутьОкно"),
	Alias(eTypeInfo       , 1, true , L"TypeInfo"       , L"Инфотип"),
};

static std::wstring param(tVariant* paParams, const long lSizeArray)
{
	std::wstring result;
	switch (TV_VT(paParams)) {
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
long CAddInNative::FindName(const std::vector<Alias>& names, const WCHAR_T* name)
{
	for (Alias alias : names) {
		for (long i = 0; i < m_AliasCount; i++) {
			if (wcsicmp(alias.Name(i), name) == 0) return alias.id;
		}
	}
	return -1;
}

//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetName(const std::vector<Alias>& names, long lPropNum, long lPropAlias)
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
long CAddInNative::GetNParams(const std::vector<Alias>& names, const long lMethodNum)
{
	for (Alias alias : names) {
		if (alias.id == lMethodNum) return alias.np;
	}
	return 0;
}
//---------------------------------------------------------------------------//
bool CAddInNative::HasRetVal(const std::vector<Alias>& names, const long lMethodNum)
{
	for (Alias alias : names) {
		if (alias.id == lMethodNum) return alias.fn;
	}
	return 0;
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
long CAddInNative::GetNProps()
{
	return ePropLast;
}

//---------------------------------------------------------------------------//
long CAddInNative::FindProp(const WCHAR_T* wsPropName)
{
	return FindName(m_PropList, wsPropName);
}

//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias)
{
	return GetName(m_PropList, lPropNum, lPropAlias);
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
		return W(ProcessManager::ProcessId(), pvarPropVal);
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
	return FindName(m_MethList, wsMethodName);
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{
	return GetName(m_MethList, lMethodNum, lMethodAlias);
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNParams(const long lMethodNum)
{
	return GetNParams(m_MethList, lMethodNum);
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
	return HasRetVal(m_MethList, lMethodNum);
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case eEnableResizing:
		return WindowsControl::EnableResizing(paParams, lSizeArray);
	case eSetWindowPos:
		return WindowsControl::SetWindowPos(paParams, lSizeArray);
	case eSetWindowSize:
		return WindowsControl::SetWindowSize(paParams, lSizeArray);
	case eSetWindowState:
		return WindowsControl::SetWindowState(paParams, lSizeArray);
	case eSetWindowText:
		return WindowsControl::SetText(paParams, lSizeArray);
	case eActivateWindow:
		return WindowsControl::Activate(paParams, lSizeArray);
	case eMaximizeWindow:
		return WindowsControl::Maximize(paParams, lSizeArray);
	case eMinimizeWindow:
		return WindowsControl::Minimize(paParams, lSizeArray);
	case eRestoreWindow:
		return WindowsControl::Restore(paParams, lSizeArray);
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool CAddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum) {
	case eFindTestClient: {
		std::wstring result;
		bool ok = W((DWORD)ProcessManager::FindTestClient(paParams, lSizeArray, result), pvarRetValue);
		if (ok && lSizeArray > 1) W(result, paParams + 1);
		return ok;
	}
	case eGetWindowList:
		return W(WindowsControl::GetWindowList(), pvarRetValue);
	case eGetProcessList:
		return W(ProcessManager::GetProcessList(paParams, lSizeArray), pvarRetValue);
	case eGetProcessInfo:
		return W(ProcessManager::GetProcessInfo(paParams, lSizeArray), pvarRetValue);
	case eGetWindowState:
		return W(WindowsControl::GetWindowState(paParams, lSizeArray), pvarRetValue);
	case eGetWindowText:
		return W(WindowsControl::GetText(paParams, lSizeArray), pvarRetValue);
	case eTakeScreenshot:
		return WindowsControl(m_iMemory).CaptureScreen(pvarRetValue, paParams, lSizeArray);
	case eCaptureWindow:
		return WindowsControl(m_iMemory).CaptureWindow(pvarRetValue, paParams, lSizeArray);
	case eTypeInfo:
		return W(paParams->vt, pvarRetValue);
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

