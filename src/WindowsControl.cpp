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
#include "ClickEffect.h"
#include "ClipboardManager.h"
#include "DesktopManager.h"
#include "FileFinder.h"
#include "ImageFinder.h"
#include "ImageHelper.h"
#include "ProcessManager.h"
#include "ScreenManager.h"
#include "SoundEffect.h"
#include "WindowsManager.h"
#include "Magnifier.h"

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

	AddFunction(u"FindTestClient", u"НайтиКлиентТестирования",
		[&](VH port) { this->result = ProcessManager::FindTestClient(port); }
	);
	AddFunction(u"GetProcessList", u"ПолучитьСписокПроцессов",
		[&](VH only1c) { this->result = ProcessManager::GetProcessList(only1c); }
	);
	AddFunction(u"GetProcessInfo", u"ПолучитьСвойстваПроцесса",
		[&](VH pid) { this->result = ProcessManager::GetProcessInfo(pid); }
	);
	AddFunction(u"ActivateProcess", u"АктивироватьПроцесс",
		[&](VH pid) { this->result = WindowsManager::ActivateProcess(pid); }
	);
	AddFunction(u"CreateProcess", u"СоздатьПроцесс",
		[&](VH cmd, VH hide) { this->result = this->LaunchProcess(cmd, hide); }, { {1, false } }
	);
	AddProcedure(u"Exit", u"ЗавершитьРаботуСистемы",
		[&](VH status) { this->ExitCurrentProcess(status); }, { {0, (int64_t)0 } }
	);
	AddFunction(u"GetProcessWindow", u"ПолучитьОкноПроцесса",
		[&](VH pid) { this->result = WindowsManager::GetTopProcessWindow(pid); }
	);
	AddFunction(u"GetTopProcessWindow", u"ПолучитьВерхнееОкноПроцесса",
		[&](VH pid) { this->result = WindowsManager::GetTopProcessWindow(pid); }
	);
	AddFunction(u"GetMainProcessWindow", u"ПолучитьГлавноеОкноПроцесса",
		[&](VH pid) { this->result = WindowsManager::GetMainProcessWindow(pid); }
	);
	AddFunction(u"GetDisplayList", u"ПолучитьСписокДисплеев",
		[&](VH window) { this->result = ScreenManager::GetDisplayList(window); }, { {0, (int64_t)0 } }
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
		[&](VH window) { this->result = WindowsManager::GetWindowInfo(window); }, { {0, (int64_t)0 } }
	);
	AddFunction(u"GetWindowSize", u"ПолучитьРазмерОкна",
		[&](VH window) { this->result = WindowsManager::GetWindowSize(window); }, { {0, (int64_t)0 } }
	);
	AddFunction(u"TakeScreenshot", u"ПолучитьСнимокЭкрана",
		[&](VH mode) { ScreenManager::CaptureScreen(this->result, mode); }, { {0, (int64_t)0} }
	);
	AddFunction(u"CaptureRegion", u"ПолучитьСнимокОбласти",
		[&](VH x, VH y, VH w, VH h) { ScreenManager::CaptureRegion(this->result, x, y, w, h); }
	);
	AddFunction(u"CaptureWindow", u"ПолучитьСнимокОкна",
		[&](VH window) { ScreenManager::CaptureWindow(this->result, window); }, { {0, (int64_t)0} }
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
		[&](VH window) { WindowsManager::Maximize(window); }, { {0, (int64_t)0 } }
	);
	AddProcedure(u"MinimizeWindow", u"СвернутьОкно",
		[&](VH window) { WindowsManager::Minimize(window); }, { {0, (int64_t)0 } }
	);
	AddProcedure(u"RestoreWindow", u"РазвернутьОкно",
		[&](VH window) { WindowsManager::Restore(window); }, { {0, (int64_t)0 } }
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
		[&](VH delay) { ScreenManager::EmulateDblClick(); }, { {0, (int64_t)100} }
	);
	AddProcedure(u"EmulateMouse", u"ЭмуляцияДвиженияМыши",
		[&](VH x, VH y, VH c, VH p) { ScreenManager::EmulateMouse(x, y, c, p, false); }
	);
	AddProcedure(u"DragAndDrop", u"ЭмуляцияПеретаскивания",
		[&](VH x, VH y, VH c, VH p) { ScreenManager::EmulateMouse(x, y, c, p, true); }
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
		[&](VH path, VH mask, VH text, VH ignore) {	this->result = FileFinder(text, ignore).find(path, mask); }, { {1, u"*.*"}, {2, u""}, {3, true} }
	);
	AddFunction(u"OutputToConsole", u"ВывестиВКонсоль",
		[&](VH text, VH encoding) { this->result = ProcessManager::ConsoleOut(text, encoding); }, { {1, (int64_t)866} }
	);
	AddFunction(u"GetFreeDiskSpace", u"СвободноеМестоНаДиске",
		[&](VH disk) { this->result = ProcessManager::GetFreeDiskSpace(disk); }
	);
	AddProcedure(u"Sleep", u"Пауза",
		[&](VH msec) { ProcessManager::Sleep(msec); }
	);
#ifdef _WINDOWS
	AddFunction(u"FindProcess", u"НайтиПроцесс",
		[&](VH query) { this->result = ProcessManager::FindProcess(query); }
	);
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
		[&](VH filename, VH async) { SoundEffect::PlaySound(filename, async); }, { {0, u""}, {1, false} }
	);
	AddProcedure(u"PlayMedia", u"ВоспроизвестиМедиа",
		[&](VH uuid, VH filename) { SoundEffect::PlayMedia(uuid, filename); }, { {1, u""} }
	);
	AddFunction(u"PlayingMedia", u"ВоспроизводитсяМедиа",
		[&](VH uuid) { this->result = SoundEffect::PlayingMedia(uuid); }
	);
	AddFunction(u"MediaCommand", u"МедиаКоманда",
		[&](VH command) { this->result = SoundEffect::MediaCommand(command); }
	);
	AddFunction(u"FindWindow", u"НайтиОкно",
		[&](VH name, VH title) { this->result = WindowsManager::FindWindow(name, title); }, { {0, u""}, { 1, u""} }
	);
	AddFunction(u"PostMessage", u"ОтправитьСообщение",
		[&](VH hWnd, VH Msg, VH wParam, VH lParam) { this->result = WindowsManager::PostMessage(hWnd, Msg, wParam, lParam); }
	);
	AddProcedure(u"DrawRectangle", u"НарисоватьПрямоугольник",
		[&](VH p, VH x, VH y, VH w, VH h) { (new RecanglePainter(p, x, y, w, h))->run(); }
	);
	AddProcedure(u"DrawEllipse", u"НарисоватьЭллипс",
		[&](VH p, VH x, VH y, VH w, VH h) { (new EllipsePainter(p, x, y, w, h))->run(); }
	);
	AddProcedure(u"DrawBezier", u"НарисоватьКривую",
		[&](VH params, VH points) { (new BezierPainter(params, points))->run(); }
	);
	AddProcedure(u"DrawArrow", u"НарисоватьСтрелку",
		[&](VH p, VH x1, VH y1, VH x2, VH y2) { (new ArrowPainter(p, x1, y1, x2, y2))->run(); }
	);
	AddProcedure(u"DrawShadow", u"НарисоватьТень",
		[&](VH p, VH x, VH y, VH w, VH h) { (new ShadowPainter(p, x, y, w, h))->run(); }
	);
	AddFunction(u"GetDesktopCount", u"ПолучитьКоличествоРабочихСтолов",
		[&]() { this->result = DesktopManager::GetDesktopCount(); }
	);
	AddFunction(u"GetCurrentDesktop", u"ПолучитьТекущийРабочийСтол",
		[&]() { this->result = DesktopManager::GetCurrentDesktopNumber(); }
	);
	AddFunction(u"CreateDesktop", u"СоздатьРабочийСтол",
		[&]() { this->result = DesktopManager::CreateDesktopNumber(); }
	);
	AddFunction(u"RemoveDesktop", u"УдалитьРабочийСтол",
		[&](VH number, VH fallback) { this->result = DesktopManager::RemoveDesktopByNumber(number, fallback); }
	);
	AddFunction(u"GoToDesktop", u"ПереключитьРабочийСтол",
		[&](VH number) { this->result = DesktopManager::GoToDesktopNumber(number); }
	);
	AddFunction(u"GetWindowDesktop", u"ПолучитьРабочийСтолОкна",
		[&](VH window) { this->result = DesktopManager::GetWindowDesktopNumber(window); }
	);
	AddFunction(u"MoveWindowToDesktop", u"ПереместитьОкноНаРабочийСтол",
		[&](VH window, VH number) { this->result = DesktopManager::MoveWindowToDesktopNumber(window, number); }
	);
	AddProcedure(u"ShowMagnifier", u"ПоказатьУвеличение",
		[&](VH x, VH y, VH w, VH h, VH z, VH f) { Magnifier::Show(x, y, w, h, z, f); },
		{ { 4, 2.0f }, { 5, (int64_t)0} }
	);
	AddProcedure(u"HideMagnifier", u"СкрытьУвеличение",
		[&]() { Magnifier::Hide(); }
	);
	AddFunction(u"ScaleImage", u"МасштабироватьИзображение",
		[&](VH image, VH factor) { ImageHelper::Scale(image, this->result, factor); }
	);
	AddFunction(u"GetElements", u"ПолучитьЭлементы",
		[&](VH pid) { this->result = uiAutomation.GetElements((DWORD)(int64_t)pid); }
	);
	AddFunction(u"FindElements", u"НайтиЭлементы",
		[&](VH pid, VH name) { this->result = uiAutomation.FindElements((DWORD)(int64_t)pid, name); }
	);
#endif//_WINDOWS

#ifdef USE_OPENCV
	AddFunction(u"FindFragment", u"НайтиФрагмент",
		[&](VH picture, VH fragment, VH method) {
			this->result = BaseHelper::ImageFinder::find(picture, fragment, method);
		}, { {2, (int64_t)1} }
		);
	AddFunction(u"FindOnScreen", u"НайтиНаЭкране",
		[&](VH fragment, VH method) {
			ScreenManager::CaptureScreen(this->result, 0);
			this->result = BaseHelper::ImageFinder::find(this->result, fragment, method);
		}, { {1, (int64_t)1} }
		);
#endif//USE_OPENCV

#ifdef USE_BOOST
	AddFunction(u"WebSocket", u"ВебСокет",
		[&](VH url, VH msg) { this->result = this->WebSocket(url, msg); }
	);
	AddFunction(u"OpenWebSocket", u"ОткрытьВебСокет",
		[&](VH url) { this->result = this->OpenWebSocket(url); }
	);
	AddFunction(u"SendWebSocket", u"ПослатьВебСокет",
		[&](VH msg) { this->result = this->SendWebSocket(msg); }
	);
	AddProcedure(u"CloseWebSocket", u"ЗакрытьВебСокет",
		[&]() { this->CloseWebSocket(); }
	);
#endif//USE_BOOST	
}

WindowsControl::~WindowsControl()
{
#ifdef USE_BOOST
	CloseWebSocket();
#endif//USE_BOOST
#ifdef _WINDOWS
	if (hProcessMonitor)
		DestroyWindow(hProcessMonitor);
	ClickEffect::Unhook();
	Magnifier::Hide();
#endif//_WINDOWS
}

static WCHAR_T* T(const std::u16string& text)
{
	return (WCHAR_T*)text.c_str();
}

#ifdef _WINDOWS

const UINT WM_PROCESS_FINISHED = WM_USER + 3;

class ProcessInfo
	: public PROCESS_INFORMATION {
private:
	HWND hWindow;
public:
	ProcessInfo(HWND hWindow) : hWindow(hWindow) {}
	void ExtrenalEvent(DWORD dwExitCode) {
		PostMessage(hWindow, WM_PROCESS_FINISHED, dwProcessId, dwExitCode);
	}
};

static DWORD WINAPI ProcessThreadProc(LPVOID lpParam)
{
	std::unique_ptr<ProcessInfo> pi((ProcessInfo*)lpParam);
	JSON json;
	DWORD dwExitCode = 0;
	json["ProcessId"] = pi->dwProcessId;
	WaitForSingleObject(pi->hProcess, INFINITE);
	if (GetExitCodeProcess(pi->hProcess, &dwExitCode))
		pi->ExtrenalEvent(dwExitCode);
	CloseHandle(pi->hProcess);
	CloseHandle(pi->hThread);
	return 0;
}

LRESULT CALLBACK ProcessWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case (WM_PROCESS_FINISHED): {
		auto component = (WindowsControl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		component->OnProcessFinished((DWORD)wParam, (DWORD)lParam);
		return 0;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

void WindowsControl::StartProcessMonitoring()
{
	if (hProcessMonitor) return;

	const LPCWSTR wsClassName = L"VanessaProcessMonitor";
	WNDCLASS wndClass = {};
	wndClass.hInstance = hModule;
	wndClass.lpfnWndProc = ProcessWndProc;
	wndClass.lpszClassName = wsClassName;
	RegisterClass(&wndClass);

	hProcessMonitor = CreateWindow(wsClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hModule, 0);
	SetWindowLongPtr(hProcessMonitor, GWLP_USERDATA, (LONG_PTR)this);
}

void WindowsControl::OnProcessFinished(DWORD ProcessId, DWORD ExitCode)
{
	JSON json;
	json["ProcessId"] = ProcessId;
	json["ExitCode"] = ExitCode;
	std::u16string data = MB2WCHAR(json.dump());
	ExternalEvent(PROCESS_FINISHED, data);
}

int64_t WindowsControl::LaunchProcess(const std::wstring& command, bool hide)
{
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	if (hide) {
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}
	int64_t result = 0;
	StartProcessMonitoring();
	ProcessInfo* pi = new ProcessInfo(hProcessMonitor);
	auto ok = CreateProcess(NULL, (LPWSTR)command.c_str(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, pi);
	if (ok) {
		result = (int64_t)pi->dwProcessId;
		CreateThread(0, NULL, ProcessThreadProc, (LPVOID)pi, NULL, NULL);
	}
	else {
		delete pi;
	}
	return result;
}

void WindowsControl::ExitCurrentProcess(int64_t status)
{
	ExitProcess((UINT)status);
}

#else//_WINDOWS

#include <sys/types.h>
#include <sys/wait.h>

IAddInDefBase* pAddInConnection = nullptr;
std::u16string wsComponentName;
std::vector<pid_t> vProcessList;

static void OnProcessTimer(int sig)
{
	for (auto it = vProcessList.begin(); it != vProcessList.end(); ) {
		int status;
		pid_t pid = *it;
		if (waitpid(pid, &status, WNOHANG) > 0) {
			JSON json;
			json["ProcessId"] = pid;
			json["ExitCode"] = WEXITSTATUS(status);
			std::u16string name = wsComponentName;
			std::u16string msg = PROCESS_FINISHED;
			std::u16string data = MB2WCHAR(json.dump());
			if (pAddInConnection)
				pAddInConnection->ExternalEvent(T(name), T(msg), T(data));
			vProcessList.erase(it);
		}
		else {
			++it;
		}
	}
}

static void OnStartProcess(pid_t pid)
{
	vProcessList.push_back(pid);
	static bool active = false;
	if (active) return;
	struct sigaction sa;
	struct itimerval tv;
	memset(&tv, 0, sizeof(tv));
	sa.sa_handler = OnProcessTimer;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &sa, NULL);
	tv.it_interval.tv_sec = 1;
	tv.it_value.tv_sec = 1;
	setitimer(ITIMER_REAL, &tv, NULL);
}

int64_t WindowsControl::LaunchProcess(const std::wstring& command, bool hide)
{
	wsComponentName = fullname();
	pAddInConnection = connection();
	std::string cmd = WC2MB(command);
	auto child = vfork();
	if (0 == child) {
		int result = execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), NULL);
		exit(result);
		return 0;
	}
	else if (child > 0) {
		OnStartProcess(child);
		return (int64_t)child;
	}
	return 0;
}

void WindowsControl::ExitCurrentProcess(int64_t status)
{
	exit((int)status);
}

#endif//_WINDOWS

#ifdef USE_BOOST

#include "WebSocket.h"

static std::wstring SocketError(const std::string& message)
{
	nlohmann::json json, j;
	j["message"] = message;
	json["error"] = j;
	return MB2WC(json.dump());
}

std::wstring WindowsControl::WebSocket(const std::string& url, const std::string& data)
{
	try {
		std::string res;
		auto msg = nlohmann::json::parse(data).dump();
		std::unique_ptr<WebSocketBase> ws(WebSocketBase::create());
		bool ok = ws->open(url, res) && ws->send(msg, res);
		return ok ? MB2WC(res) : SocketError(res);
	}
	catch (nlohmann::json::parse_error&) {
		return SocketError("JSON parse error");
	}
}

std::wstring WindowsControl::OpenWebSocket(const std::string& url)
{
	std::string res;
	std::unique_ptr<WebSocketBase> ws(WebSocketBase::create());
	if (ws->open(url, res)) {
		webSocket = std::move(ws);
		return MB2WC(res);
	}
	else {
		return SocketError(res);
	}
}

std::wstring WindowsControl::SendWebSocket(const std::string& data)
{
	if (!webSocket)
		return SocketError("Error: WebSocket closed");
	try {
		std::string res;
		auto msg = nlohmann::json::parse(data).dump();
		return webSocket->send(msg, res) ? MB2WC(res) : SocketError(res);
	}
	catch (nlohmann::json::parse_error&) {
		return SocketError("JSON parse error");
	}
}

#endif //USE_BOOST
