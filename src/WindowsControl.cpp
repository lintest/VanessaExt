#include "stdafx.h"

#ifndef _WINDOWS
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <iconv.h>
#include <sys/time.h>
#endif //_WINDOWS

#include "WindowsControl.h"
#include "version.h"

#include "ClickEffect.h"
#include "ClipboardManager.h"
#include "FileFinder.h"
#include "ProcessManager.h"
#include "ScreenManager.h"
#include "WindowsManager.h"

std::vector<std::u16string> WindowsControl::names = {
	AddComponent(u"WindowsControl", []() { return new WindowsControl; }),
};

WindowsControl::WindowsControl() {

	AddProperty(u"ClipboardText", u"ТекстБуфераОбмена",
		[&](VH var) { var = ClipboardManager().GetText(); },
		[&](VH var) { ClipboardManager().SetText(var); }
	);
	AddProperty(u"ClipboardImage", u"КартинкаБуфераОбмена",
		[&](VH var) { ClipboardManager().GetImage(var); },
		[&](VH var) { ClipboardManager().SetImage(var); }
	);
	AddProperty(u"ClipboardFormat", u"ФорматБуфераОбмена",
		[&](VH var) { var = ClipboardManager().GetFormat(); }
	);
	AddProperty(u"ActiveWindow", u"АктивноеОкно",
		[&](VH var) { var = WindowsManager::ActiveWindow(); },
		[&](VH var) { WindowsManager::Activate(var); }
	);
	AddProperty(u"ProcessId", u"ИдентификаторПроцесса",
		[&](VH var) { var = ProcessManager::ProcessId(); }
	);
	AddProperty(u"CursorPos", u"ПозицияКурсора",
		[&](VH var) { var = ScreenManager::GetCursorPos(); }
	);
	AddProperty(u"WindowList", u"СписокОкон",
		[&](VH var) { var = WindowsManager::GetWindowList(0); }
	);
	AddProperty(u"ProcessList", u"СписокПроцессов",
		[&](VH var) { var = ProcessManager::GetProcessList(false); }
	);
	AddProperty(u"DisplayList", u"СписокДисплеев",
		[&](VH var) { var = ScreenManager::GetDisplayList(0); }
	);
	AddProperty(u"ScreenInfo", u"СвойстваЭкрана",
		[&](VH var) { var = ScreenManager::GetScreenInfo(); }
	);
	AddProperty(u"Version", u"Версия",
		[&](VH var) { var = std::string(VER_FILE_VERSION_STR); }
	);

	AddFunction(u"FindTestClient", u"НайтиКлиентТестирования",
		[&](VH port) { this->result = ProcessManager::FindTestClient(port); }
	);
	AddFunction(u"GetProcessList", u"ПолучитьСписокПроцессов",
		[&](VH only1c) { this->result = ProcessManager::GetProcessList(only1c); }
	);
	AddFunction(u"GetProcessInfo", u"ПолучитьСвойстваПроцесса",
		[&](VH pid) { this->result = ProcessManager::GetProcessInfo(pid); }
	);
	AddFunction(u"GetDisplayList", u"ПолучитьСписокДисплеев",
		[&](VH window) { this->result = ScreenManager::GetDisplayList(window); }
	);
	AddFunction(u"GetDisplayInfo", u"ПолучитьСвойстваДисплея",
		[&](VH window) { this->result = ScreenManager::GetDisplayInfo(window); }
	);
	AddFunction(u"GetScreenInfo", u"ПолучитьСвойстваЭкрана",
		[&]() { this->result = ScreenManager::GetScreenInfo(); }
	);
	AddFunction(u"GetWindowList", u"ПолучитьСписокОкон",
		[&](VH pid) { this->result = WindowsManager::GetWindowList(pid); }, { {0, (int64_t)0 } }
	);
	AddFunction(u"GetWindowInfo", u"ПолучитьСвойстваОкна",
		[&](VH window) { this->result = WindowsManager::GetWindowInfo(window); }
	);
	AddFunction(u"GetWindowSize", u"ПолучитьРазмерОкна",
		[&](VH window) { this->result = WindowsManager::GetWindowSize(window); }
	);
	AddFunction(u"TakeScreenshot", u"ПолучитьСнимокЭкрана",
		[&](VH mode) { ScreenManager::CaptureScreen(this->result, mode); }
	);
	AddFunction(u"CaptureWindow", u"ПолучитьСнимокОкна",
		[&](VH window) { ScreenManager::CaptureWindow(this->result, window); }
	);
	AddFunction(u"CaptureProcess", u"ПолучитьСнимокПроцесса",
		[&](VH pid) { ScreenManager::CaptureProcess(this->result, pid); }
	);
	AddProcedure(u"EnableResizing", u"РазрешитьИзменятьРазмер",
		[&](VH window, VH enable) { WindowsManager::EnableResizing(window, enable); }
	);
	AddProcedure(u"SetWindowPos", u"УстановитьПозициюОкна",
		[&](VH window, VH x, VH y, VH w, VH h) { WindowsManager::SetWindowPos(window, x, y, w, h); }, { {3, (int64_t)0}, {4, (int64_t)0} }
	);
	AddProcedure(u"SetWindowSize", u"УстановитьРазмерОкна",
		[&](VH window, VH w, VH h) { WindowsManager::SetWindowSize(window, w, h); }
	);
	AddProcedure(u"SetWindowState", u"УстановитьСтатусОкна",
		[&](VH window, VH state, VH activate) { WindowsManager::SetWindowState(window, state, activate); }
	);
	AddProcedure(u"ActivateWindow", u"АктивироватьОкно",
		[&](VH window) { WindowsManager::Activate(window); }
	);
	AddProcedure(u"MaximizeWindow", u"РаспахнутьОкно",
		[&](VH window) { WindowsManager::Maximize(window); }
	);
	AddProcedure(u"MinimizeWindow", u"СвернутьОкно",
		[&](VH window) { WindowsManager::Minimize(window); }
	);
	AddProcedure(u"RestoreWindow", u"РазвернутьОкно",
		[&](VH window) { WindowsManager::Restore(window); }
	);
	AddProcedure(u"EmptyClipboard", u"ОчиститьБуферОбмена",
		[&]() { ClipboardManager().Empty(); }
	);
	AddFunction(u"GetCursorPos", u"ПолучитьПозициюКурсора",
		[&]() { this->result = ScreenManager::GetCursorPos(); }
	);
	AddProcedure(u"SetCursorPos", u"УстановитьПозициюКурсора",
		[&](VH x, VH y) { ScreenManager::SetCursorPos(x, y); }
	);
	AddProcedure(u"EmulateClick", u"ЭмуляцияНажатияМыши",
		[&](VH button, VH flags) { ScreenManager::EmulateClick(button, flags); }, { {0, (int64_t)0}, {1, (int64_t)0} }
	);
	AddProcedure(u"EmulateDblClick", u"ЭмуляцияДвойногоНажатия",
		[&]() { ScreenManager::EmulateDblClick(); }
	);
	AddProcedure(u"EmulateMouse", u"ЭмуляцияДвиженияМыши",
		[&](VH x, VH y, VH c, VH p) { ScreenManager::EmulateMouse(x, y, c, p); }
	);
	AddProcedure(u"EmulateWheel", u"ЭмуляцияКолесаМыши",
		[&](VH sign, VH flags) { ScreenManager::EmulateWheel(sign, flags); }
	);
	AddProcedure(u"EmulateHotkey", u"ЭмуляцияНажатияКлавиши",
		[&](VH keys, VH flags) { ScreenManager::EmulateHotkey(keys, flags); }, { {1, (int64_t)0} }
	);
	AddProcedure(u"EmulateText", u"ЭмуляцияВводаТекста",
		[&](VH text, VH pause) { ScreenManager::EmulateText(text, pause); }, { {1, (int64_t)0} }
	);
	AddFunction(u"FindFiles", u"НайтиФайлы",
		[&](VH path, VH mask, VH text, VH ignore) {	this->result = FileFinder(text, ignore).find(path, mask); }, { {3, true} }
	);
	AddProcedure(u"Sleep", u"Пауза",
		[&](VH msec) { ProcessManager::Sleep(msec); }
	);
#ifdef _WINDOWS
	AddProcedure(u"ShowClick", u"ПоказатьНажатиеМыши",
		[&](VH color, VH radius, VH width, VH delay, VH trans) { ClickEffect::Show(color, radius, width, delay, trans); },
		{ {0, (int64_t)RGB(200, 50, 50)}, {1, (int64_t)30}, {2, (int64_t)12}, {3, (int64_t)12}, {4, (int64_t)127} }
	);
	AddProcedure(u"VisualizeClick", u"ВизуализацияНажатияМыши",
		[&](VH color, VH radius, VH width, VH delay, VH trans) { ClickEffect::Hook(color, radius, width, delay, trans); },
		{ {0, (int64_t)RGB(200, 50, 50)}, {1, (int64_t)30}, {2, (int64_t)12}, {3, (int64_t)12}, {4, (int64_t)127} }
	);
	AddProcedure(u"ShowClickVisualization", u"ПоказатьВизуализациюНажатияМыши",
		[&]() { ClickEffect::Show(); }
	);
	AddProcedure(u"StopClickVisualization", u"ПрекратитьВизуализациюНажатияМыши",
		[&]() { ClickEffect::Unhook(); }
	);
	AddProcedure(u"PlaySound", u"ВоспроизвестиЗвук",
		[&](VH filename, VH async) { ProcessManager::PlaySound(filename, async); }, { {0, u""}, {1, false} }
	);
	AddFunction(u"MediaCommand", u"МедиаКоманда",
		[&](VH command) { this->result = ProcessManager::MediaCommand(command); }
	);
	AddFunction(u"WebSocket", u"ВебСокет",
		[&](VH url, VH msg) { this->result = ProcessManager::WebSocket(url, msg); }
	);
	AddFunction(u"OpenWebSocket", u"ОткрытьВебСокет",
		[&](VH url) { this->result = ProcessManager::OpenWebSocket(&webSocket, url); }
	);
	AddFunction(u"SendWebSocket", u"ПослатьВебСокет",
		[&](VH msg) { this->result = ProcessManager::SendWebSocket(&webSocket, msg); }
	);
	AddFunction(u"FindWindow", u"НайтиОкно",
		[&](VH name, VH title) { this->result = WindowsManager::FindWindow(name, title); }, { {0, u""}, { 1, u""} }
	);
	AddFunction(u"PostMessage", u"ОтправитьСообщение",
		[&](VH hWnd, VH Msg, VH wParam, VH lParam) { this->result = WindowsManager::PostMessage(hWnd, Msg, wParam, lParam); }
	);
	AddProcedure(u"CloseWebSocket", u"ЗакрытьВебСокет",
		[&](VH msec) { this->CloseWebSocket(); }
	);
#endif//_WINDOWS
}

WindowsControl::~WindowsControl()
{
#ifdef _WINDOWS
	ClickEffect::Unhook();
#endif//_WINDOWS
}

