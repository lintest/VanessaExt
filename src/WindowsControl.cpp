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

#include "WindowsControl.h"
#include "version.h"

#include "ClipMngr.h"
#include "ProcMngr.h"
#include "ScreenMngr.h"
#include "WindowMngr.h"

const wchar_t* WindowsControl::m_ExtensionName = L"WindowsControl";

const std::vector<AddInBase::Alias> WindowsControl::m_PropList{
	Alias(eClipboardImage  , true  , L"ClipboardImage"  , L"КартинкаБуфераОбмена"),
	Alias(eClipboardText   , true  , L"ClipboardText"   , L"ТекстБуфераОбмена"),
	Alias(eClipboardFormat , false , L"ClipboardFormat" , L"ФорматБуфераОбмена"),
	Alias(eActiveWindow    , true  , L"ActiveWindow"    , L"АктивноеОкно"),
	Alias(eProcessId       , false , L"ProcessId"       , L"ИдентификаторПроцесса"),
	Alias(eWindowList      , false , L"WindowList"      , L"СписокОкон"),
	Alias(eProcessList     , false , L"ProcessList"     , L"СписокПроцессов"),
	Alias(eDisplayList     , false , L"DisplayList"     , L"СписокДисплеев"),
	Alias(eScreenInfo      , false , L"ScreenInfo"      , L"СвойстваЭкрана"),
	Alias(eCursorPos       , false , L"CursorPos"       , L"ПозицияКурсора"),
	Alias(eVersion         , false , L"Version"         , L"Версия"),
};

const std::vector<AddInBase::Alias> WindowsControl::m_MethList{
	Alias(eFindTestClient  , 1, true , L"FindTestClient"   , L"НайтиКлиентТестирования"),
	Alias(eGetProcessList  , 1, true , L"GetProcessList"   , L"ПолучитьСписокПроцессов"),
	Alias(eGetProcessInfo  , 1, true , L"GetProcessInfo"   , L"ПолучитьСвойстваПроцесса"),
	Alias(eGetDisplayList  , 1, true , L"GetDisplayList"   , L"ПолучитьСписокДисплеев"),
	Alias(eGetDisplayInfo  , 1, true , L"GetDisplayInfo"   , L"ПолучитьСвойстваДисплея"),
	Alias(eGetScreenInfo   , 0, true , L"GetScreenInfo"    , L"ПолучитьСвойстваЭкрана"),
	Alias(eGetWindowList   , 1, true , L"GetWindowList"    , L"ПолучитьСписокОкон"),
	Alias(eGetWindowInfo   , 1, true , L"GetWindowInfo"    , L"ПолучитьСвойстваОкна"),
	Alias(eGetWindowSize   , 1, true , L"GetWindowSize"    , L"ПолучитьРазмерОкна"),
	Alias(eTakeScreenshot  , 1, true , L"TakeScreenshot"   , L"ПолучитьСнимокЭкрана"),
	Alias(eCaptureWindow   , 1, true , L"CaptureWindow"    , L"ПолучитьСнимокОкна"),
	Alias(eCaptureProcess  , 1, true , L"CaptureProcess"   , L"ПолучитьСнимокПроцесса"),
	Alias(eEnableResizing  , 2, false, L"EnableResizing"   , L"РазрешитьИзменятьРазмер"),
	Alias(eSetWindowPos    , 3, false, L"SetWindowPos"     , L"УстановитьПозициюОкна"),
	Alias(eSetWindowSize   , 3, false, L"SetWindowSize"    , L"УстановитьРазмерОкна"),
	Alias(eSetWindowState  , 3, false, L"SetWindowState"   , L"УстановитьСтатусОкна"),
	Alias(eActivateWindow  , 1, false, L"ActivateWindow"   , L"АктивироватьОкно"),
	Alias(eMaximizeWindow  , 1, false, L"MaximizeWindow"   , L"РаспахнутьОкно"),
	Alias(eMinimizeWindow  , 1, false, L"MinimizeWindow"   , L"СвернутьОкно"),
	Alias(eRestoreWindow   , 1, false, L"RestoreWindow"    , L"РазвернутьОкно"),
	Alias(eEmptyClipboard  , 0, false, L"EmptyClipboard"   , L"ОчиститьБуферОбмена"),
	Alias(eGetCursorPos    , 0, true , L"GetCursorPos"     , L"ПолучитьПозициюКурсора"),
	Alias(eSetCursorPos    , 2, false, L"SetCursorPos"     , L"УстановитьПозициюКурсора"),
	Alias(eMoveCursorPos   , 3, false, L"MoveCursorPos"    , L"ПереместитьПозициюКурсора"),
	Alias(eEmulateClick    , 1, false, L"EmulateClick"     , L"ЭмуляцияНажатияМыши"),
	Alias(eEmulateDblClick , 0, false, L"EmulateDblClick"  , L"ЭмуляцияДвойногоНажатия"),
	Alias(eEmulateMouse    , 4, false, L"EmulateMouse"     , L"ЭмуляцияДвиженияМыши"),
 	Alias(eEmulateHotkey   , 2, false, L"EmulateHotkey"    , L"ЭмуляцияНажатияКлавиши"),
	Alias(eEmulateText     , 2, false, L"EmulateText"      , L"ЭмуляцияВводаТекста"),
	Alias(eWebSocket       , 2, true,  L"WebSocket"        , L"ВебСокет"),
	Alias(eSleep           , 1, false, L"Sleep"            , L"Пауза"),
};

/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool WindowsControl::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum) {
	case eClipboardImage:
		return ClipboardManager(this).GetImage(pvarPropVal);
	case eClipboardText:
		return VA(pvarPropVal) << ClipboardManager(this).GetText();
	case eClipboardFormat:
		return VA(pvarPropVal) << ClipboardManager(this).GetFormat();
	case eActiveWindow:
		return VA(pvarPropVal) << (int64_t)WindowManager::ActiveWindow();
	case eProcessList:
		return VA(pvarPropVal) << ProcessManager::GetProcessList(NULL, 0);
	case eWindowList:
		return VA(pvarPropVal) << WindowManager::GetWindowList(NULL, 0);
	case eDisplayList:
		return VA(pvarPropVal) << ScreenManager::GetDisplayList(NULL, 0);
	case eScreenInfo:
		return VA(pvarPropVal) << ScreenManager::GetScreenInfo();
	case eCursorPos:
		return VA(pvarPropVal) << ScreenManager::GetCursorPos();
	case eProcessId:
		return VA(pvarPropVal) << ProcessManager::ProcessId();
	case eVersion:
		return VA(pvarPropVal) << MB2WC(VER_FILE_VERSION_STR);
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool WindowsControl::SetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum) {
	case eClipboardImage:
		return ClipboardManager(this).SetImage(pvarPropVal);
	case eClipboardText:
		return ClipboardManager(this).SetText(pvarPropVal);
	case eActiveWindow:
		return WindowManager::Activate(pvarPropVal, 1);
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool WindowsControl::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case eSetWindowPos:
		return WindowManager::SetWindowPos(paParams, lSizeArray);
	case eSetWindowSize:
		return WindowManager::SetWindowSize(paParams, lSizeArray);
	case eActivateWindow:
		return WindowManager::Activate(paParams, lSizeArray);
	case eMaximizeWindow:
		return WindowManager::Maximize(paParams, lSizeArray);
	case eMinimizeWindow:
		return WindowManager::Minimize(paParams, lSizeArray);
	case eRestoreWindow:
		return WindowManager::Restore(paParams, lSizeArray);
	case eEnableResizing:
		return WindowManager::EnableResizing(paParams, lSizeArray);
	case eEmptyClipboard:
		return ClipboardManager(this).Empty();
	case eSetCursorPos:
		return ScreenManager::SetCursorPos(paParams, lSizeArray);
	case eMoveCursorPos:
		return ScreenManager::MoveCursorPos(paParams, lSizeArray);
	case eEmulateClick:
		return ScreenManager::EmulateClick(paParams, lSizeArray);
	case eEmulateDblClick:
		return ScreenManager::EmulateDblClick(paParams, lSizeArray);
	case eEmulateMouse:
		return ScreenManager::EmulateMouse(paParams, lSizeArray);
	case eEmulateHotkey:
		return ScreenManager::EmulateHotkey(paParams, lSizeArray);
	case eEmulateText:
		return ScreenManager::EmulateText(paParams, lSizeArray);
	case eSleep:
		return ProcessManager::Sleep(paParams, lSizeArray);
#ifdef _WINDOWS
	case eSetWindowState:
		return WindowManager::SetWindowState(paParams, lSizeArray);
#endif
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool WindowsControl::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum) {
	case eFindTestClient:
		return VA(pvarRetValue) << ProcessManager::FindTestClient(paParams, lSizeArray);
	case eGetScreenInfo:
		return VA(pvarRetValue) << ScreenManager::GetScreenInfo();
	case eGetProcessList:
		return VA(pvarRetValue) << ProcessManager::GetProcessList(paParams, lSizeArray);
	case eGetProcessInfo:
		return VA(pvarRetValue) << ProcessManager::GetProcessInfo(paParams, lSizeArray);
	case eGetWindowList:
		return VA(pvarRetValue) << WindowManager::GetWindowList(paParams, lSizeArray);
	case eGetWindowInfo:
		return VA(pvarRetValue) << WindowManager::GetWindowInfo(paParams, lSizeArray);
	case eGetWindowSize:
		return VA(pvarRetValue) << WindowManager::GetWindowSize(paParams, lSizeArray);
	case eGetDisplayList:
		return VA(pvarRetValue) << ScreenManager::GetDisplayList(paParams, lSizeArray);
	case eGetDisplayInfo:
		return VA(pvarRetValue) << ScreenManager::GetDisplayInfo(paParams, lSizeArray);
	case eTakeScreenshot:
		return ScreenManager(this).CaptureScreen(pvarRetValue, paParams, lSizeArray);
	case eCaptureWindow:
		return ScreenManager(this).CaptureWindow(pvarRetValue, paParams, lSizeArray);
	case eCaptureProcess:
		return ScreenManager(this).CaptureProcess(pvarRetValue, paParams, lSizeArray);
	case eGetCursorPos:
		return VA(pvarRetValue) << ScreenManager::GetCursorPos();
	case eWebSocket:
		return VA(pvarRetValue) << ProcessManager::WebSocket(paParams, lSizeArray);
	default:
		return false;
	}
}

