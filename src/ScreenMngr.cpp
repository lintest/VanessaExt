#include "stdafx.h"
#include <math.h>
#include "ScreenMngr.h"
#include "json_ext.h"

BOOL ScreenManager::CaptureWindow(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = (HWND)VarToInt(paParams);
	return CaptureWindow(pvarRetValue, hWnd);
}

#ifdef _WINDOWS

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include "ImageHelper.h"

nlohmann::json RectToJson(const RECT& rect)
{
	nlohmann::json json;
	json["Left"] = rect.left;
	json["Top"] = rect.top;
	json["Right"] = rect.right;
	json["Bottom"] = rect.bottom;
	json["Width"] = rect.right - rect.left;
	json["Height"] = rect.bottom - rect.top;
	return json;
}

std::wstring ScreenManager::GetDisplayList(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
	if (!IsWindow(hWnd)) hWnd = 0;

	HDC hdc = NULL;
	RECT rect;
	LPCRECT lpRect = NULL;
	if (hWnd) {
		GetClientRect(hWnd, &rect);
		hdc = GetDC(hWnd);
		lpRect = &rect;
	}

	nlohmann::json json;
	BOOL bResult = ::EnumDisplayMonitors(hdc, lpRect, [](HMONITOR hMonitor, HDC, LPRECT rect, LPARAM lParam) -> BOOL
		{
			nlohmann::json j = RectToJson(*rect);
			MONITORINFOEX mi;
			mi.cbSize = sizeof(mi);
			if (::GetMonitorInfo(hMonitor, &mi)) {
				j["Name"] = WC2MB((mi.szDevice));
				j["Work"] = RectToJson(mi.rcWork);
			}
			nlohmann::json* json = (JSON*)lParam;
			json->push_back(j);
			return TRUE;
		}, (LPARAM)&json);

	if (hdc) ReleaseDC(hWnd, hdc);
	return MB2WC(json.dump());
}

std::wstring ScreenManager::GetDisplayInfo(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
	if (!hWnd) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return {};

	RECT rect;
	GetWindowRect(hWnd, &rect);
	HMONITOR hMonitor = ::MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	if (!hMonitor) return {};

	MONITORINFOEX mi;
	mi.cbSize = sizeof(mi);
	BOOL bResult = ::GetMonitorInfo(hMonitor, &mi);
	if (!bResult) return {};

	nlohmann::json json = RectToJson(mi.rcMonitor);
	json["Name"] = WC2MB((mi.szDevice));
	json["Work"] = RectToJson(mi.rcWork);
	return MB2WC(json.dump());
}

std::wstring ScreenManager::GetScreenInfo()
{
	RECT rect, work{ 0,0,0,0 };
	rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
	rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
	nlohmann::json json = RectToJson(rect);
	json["Work"] = RectToJson(work);
	return MB2WC(json.dump());
}

std::wstring ScreenManager::GetScreenList()
{
	return {};
}

BOOL ScreenManager::CaptureScreen(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	const int mode = lSizeArray == 0 ? 0 : VarToInt(paParams);
	if (mode) return CaptureWindow(pvarRetValue, 0);

	HWND hWnd = ::GetForegroundWindow();
	if (IsWindow(hWnd)) UpdateWindow(hWnd);

	LONG w = GetSystemMetrics(SM_CXSCREEN);
	LONG h = GetSystemMetrics(SM_CYSCREEN);

	HDC hScreen = GetDC(NULL);
	HDC hDC = CreateCompatibleDC(hScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
	HGDIOBJ object = SelectObject(hDC, hBitmap);
	BitBlt(hDC, 0, 0, w, h, hScreen, 0, 0, SRCCOPY);
	ImageHelper(hBitmap).Save(m_addin, pvarRetValue);
	SelectObject(hDC, object);
	DeleteDC(hDC);
	ReleaseDC(NULL, hScreen);
	DeleteObject(hBitmap);
	return true;
}

BOOL ScreenManager::CaptureWindow(tVariant* pvarRetValue, HWND hWnd)
{
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	HDC hdcScreen = GetDC(NULL);
	HDC hDC = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, rc.right - rc.left, rc.bottom - rc.top);
	SelectObject(hDC, hBitmap);
	::PrintWindow(hWnd, hDC, PW_CLIENTONLY);
	ImageHelper(hBitmap).Save(m_addin, pvarRetValue);
	ReleaseDC(NULL, hdcScreen);
	DeleteDC(hDC);
	DeleteObject(hBitmap);
	return true;
}

BOOL ScreenManager::CaptureProcess(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	class Param {
	public:
		DWORD pid = 0;
		std::map<HWND, bool> map;
	};
	Param p;
	p.pid = VarToInt(paParams);
	bool bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			if (::IsWindow(hWnd) && ::IsWindowVisible(hWnd)) {
				Param* p = (Param*)lParam;
				DWORD dwProcessId;
				WCHAR buffer[256];
				::GetWindowThreadProcessId(hWnd, &dwProcessId);
				if (p->pid == dwProcessId
					&& ::GetClassName(hWnd, buffer, 256)
					&& (wcscmp(L"V8TopLevelFrameSDIsec", buffer) == 0
						|| wcscmp(L"V8TopLevelFrameSDI", buffer) == 0
						|| wcscmp(L"V8TopLevelFrame", buffer) == 0)
					) {
					if (p->map.find(hWnd) == p->map.end()) p->map[hWnd] = true;
					HWND hParent = ::GetWindow(hWnd, GW_OWNER);
					if (hParent) p->map[hParent] = false;
				}
			}
			return TRUE;
		}, (LPARAM)&p);
	for (auto it = p.map.begin(); it != p.map.end(); it++) {
		if (it->second) return CaptureWindow(pvarRetValue, it->first);
	}
	return true;
}

std::wstring ScreenManager::GetCursorPos()
{
	POINT pos;
	::GetCursorPos(&pos);
	JSON json;
	json["x"] = pos.x;
	json["y"] = pos.y;
	return json;
}

BOOL ScreenManager::SetCursorPos(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 2) return false;
	const int x = VarToInt(paParams);
	const int y = VarToInt(paParams + 1);
	return ::SetCursorPos(x, y);
}

class Hotkey
	: private std::vector<INPUT>
{
public:
	void add(WORD key) {
		INPUT ip;
		::ZeroMemory(&ip, sizeof(ip));
		ip.type = INPUT_KEYBOARD;
		ip.ki.wVk = key;
		push_back(ip);
	}
	bool add(std::wstring text) {
		try {
			auto json = JSON::parse(WC2MB(text));
			for (JSON::iterator it = json.begin(); it != json.end(); ++it) {
				add((WORD)it.value());
			}
		}
		catch (nlohmann::json::parse_error e) {
			return false;
		}
		return true;
	}
	void send() {
		if (size() == 0) return;
		SendInput((UINT)size(), data(), sizeof(INPUT));
		std::reverse(begin(), end());
		for (auto it = begin(); it != end(); ++it) {
			it->ki.dwFlags = KEYEVENTF_KEYUP;
		}
		SendInput((UINT)size(), data(), sizeof(INPUT));
	}
	void down() {
		if (size() == 0) return;
		SendInput((UINT)size(), data(), sizeof(INPUT));
	}
	void up() {
		std::reverse(begin(), end());
		for (auto it = begin(); it != end(); ++it) {
			it->ki.dwFlags = KEYEVENTF_KEYUP;
		}
		SendInput((UINT)size(), data(), sizeof(INPUT));
	}
};

BOOL ScreenManager::EmulateDblClick(tVariant* paParams, const long lSizeArray)
{
	DWORD dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
	mouse_event(dwFlags, 0, 0, 0, 0);
	mouse_event(dwFlags, 0, 0, 0, 0);
	return true;
}

BOOL ScreenManager::EmulateClick(tVariant* paParams, const long lSizeArray)
{
	DWORD dwFlags;
	switch (VarToInt(paParams)) {
	case 1: dwFlags = MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP; break;
	case 2: dwFlags = MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP; break;
	case 3: dwFlags = MOUSEEVENTF_XDOWN | MOUSEEVENTF_XUP; break;
	default: dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
	}
	Hotkey hotkey;
	tVariant* pvarKeys = paParams + 1;
	if (TV_VT(pvarKeys) == VTYPE_PWSTR) {
		if (pvarKeys->pwstrVal == nullptr) return false;
		if (!hotkey.add(VarToStr(pvarKeys))) return false;
	}
	else if (TV_VT(pvarKeys) == VTYPE_I4) {
		WORD flags = VarToInt(pvarKeys);
		if (flags & 0x04) hotkey.add(VK_SHIFT);
		if (flags & 0x08) hotkey.add(VK_CONTROL);
		if (flags & 0x10) hotkey.add(VK_MENU);
	}
	hotkey.down();
	mouse_event(dwFlags, 0, 0, 0, 0);
	hotkey.up();
	return true;
}

BOOL ScreenManager::EmulateWheel(tVariant* paParams, const long lSizeArray)
{
	Hotkey hotkey;
	tVariant* pvarKeys = paParams + 1;
	if (TV_VT(pvarKeys) == VTYPE_PWSTR) {
		if (pvarKeys->pwstrVal == nullptr) return false;
		if (!hotkey.add(VarToStr(pvarKeys))) return false;
	}
	else if (TV_VT(pvarKeys) == VTYPE_I4) {
		WORD flags = VarToInt(pvarKeys);
		if (flags & 0x04) hotkey.add(VK_SHIFT);
		if (flags & 0x08) hotkey.add(VK_CONTROL);
		if (flags & 0x10) hotkey.add(VK_MENU);
	}
	hotkey.down();
	DWORD delta = VarToInt(paParams) < 0 ? -WHEEL_DELTA : WHEEL_DELTA;
	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, delta, 0);
	hotkey.up();
	return true;
}

BOOL ScreenManager::EmulateMouse(tVariant* paParams, const long lSizeArray)
{
	double x = VarToInt(paParams);
	double y = VarToInt(paParams + 1);
	double count = VarToInt(paParams + 2);
	DWORD pause = VarToInt(paParams + 3);

	POINT p;
	::GetCursorPos(&p);
	double dx = x - p.x;
	double dy = y - p.y;
	int px = 3, py = 2;
	if (abs(dx) < abs(dy)) {
		px = 2; py = 3;
	}
	count /= 2;
	double cx = pow(count, px) * 2;
	double cy = pow(count, py) * 2;
	double fx = 65535.0f / (::GetSystemMetrics(SM_CXSCREEN) - 1);
	double fy = 65535.0f / (::GetSystemMetrics(SM_CYSCREEN) - 1);

	INPUT ip;
	::ZeroMemory(&ip, sizeof(ip));
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
	for (double i = 1; i <= count; i++) {
		ip.mi.dx = LONG(fx * (p.x + dx * pow(i, px) / cx));
		ip.mi.dy = LONG(fy * (p.y + dy * pow(i, py) / cy));
		SendInput(1, &ip, sizeof(INPUT));
		if (pause) Sleep(pause);
	}
	for (double i = count - 1; i >= 0; i--) {
		ip.mi.dx = LONG(fx * (x - dx * pow(i, px) / cx));
		ip.mi.dy = LONG(fy * (y - dy * pow(i, py) / cy));
		SendInput(1, &ip, sizeof(INPUT));
		if (pause) Sleep(pause);
	}

	return true;
}

BOOL ScreenManager::EmulateHotkey(tVariant* paParams, const long lSizeArray)
{
	Sleep(100);
	Hotkey hotkey;
	if (TV_VT(paParams) == VTYPE_PWSTR) {
		if (paParams->pwstrVal == nullptr) return false;
		if (!hotkey.add(VarToStr(paParams))) return false;
	}
	else {
		WORD key = VarToInt(paParams);
		WORD flags = VarToInt(paParams + 1);
		if (flags & 0x04) hotkey.add(VK_SHIFT);
		if (flags & 0x08) hotkey.add(VK_CONTROL);
		if (flags & 0x10) hotkey.add(VK_MENU);
		if (key) hotkey.add(key);
	}
	hotkey.send();
	return true;
}

BOOL ScreenManager::EmulateText(tVariant* paParams, const long lSizeArray)
{
	Sleep(100);
	std::wstring text = VarToStr(paParams);
	DWORD pause = VarToInt(paParams + 1);
	for (auto ch : text) {
		INPUT ip;
		::ZeroMemory(&ip, sizeof(ip));
		ip.type = INPUT_KEYBOARD;
		ip.ki.dwFlags = KEYEVENTF_UNICODE;
		ip.ki.wScan = ch;
		SendInput(1, &ip, sizeof(INPUT));
		ip.ki.dwFlags |= KEYEVENTF_KEYUP;
		SendInput(1, &ip, sizeof(INPUT));
		if (pause) Sleep(pause);
	}
	return true;
}

#else //_WINDOWS

#include "screenshot.h"
#include "XWinBase.h"
#include <X11/extensions/XTest.h>
#include <unistd.h>

class ScreenEnumerator : public WindowHelper
{
public:
	std::wstring Enumerate() {
		int count_screens = ScreenCount(display);
		for (int i = 0; i < count_screens; ++i) {
			Screen* screen = ScreenOfDisplay(display, i);
			JSON j;
			j["Id"] = i;
			j["Width"] = screen->width;
			j["Height"] = screen->height;
			json.push_back(j);
		}
		return json;
	}
};

static void Assign(JSON& json, Display* display, XRRMonitorInfo* info)
{
	char* name = XGetAtomName(display, info->name);
	json["Name"] = name;
	json["Left"] = info->x;
	json["Top"] = info->y;
	json["Width"] = info->width;
	json["Height"] = info->height;
	json["mWidth"] = info->mwidth;
	json["mHeight"] = info->mheight;
	json["Right"] = info->x + info->width;
	json["Bottom"] = info->y + info->height;
	json["Automatic"] = (bool)info->automatic;
	json["Primary"] = (bool)info->primary;
	XFree(name);
}

class DisplayEnumerator : public WindowHelper
{
public:
	std::wstring Enumerate(Window window) {
		int	count;
		Rect rect = Rect(display, window);
		Window root = DefaultRootWindow(display);
		XRRMonitorInfo* monitors = XRRGetMonitors(display, root, false, &count);
		if (count == -1 || monitors == NULL) return {};
		for (int i = 0; i < count; ++i) {
			if (window != 0) {
				int s = rect * Rect(monitors + i);
				if (s == 0) continue;
			}
			JSON j;
			Assign(j, display, monitors + i);
			json.push_back(j);
		}
		XFree(monitors);
		return json;
	}
};

class DisplayFinder : public WindowHelper
{
public:
	std::wstring FindDisplay(Window window) {
		Rect rect = Rect(display, window);
		if (window == 0) window = GetActiveWindow();
		int	count = -1, result = -1; int sq = 0;
		Window root = DefaultRootWindow(display);
		XRRMonitorInfo* monitors = XRRGetMonitors(display, root, false, &count);
		if (count == -1 || monitors == NULL) return {};
		for (int i = 0; i < count; ++i) {
			int s = rect * Rect(monitors + i);
			if (s > sq) { result = i; sq = s; }
		}
		if (result >= 0) {
			Assign(json, display, monitors + result);
		}
		XFree(monitors);
		return json;
	}
};

std::wstring ScreenManager::GetScreenList()
{
	return ScreenEnumerator().Enumerate();
}

std::wstring ScreenManager::GetDisplayList(tVariant* paParams, const long lSizeArray)
{
	Window window = 0;
	if (lSizeArray > 0) window = VarToInt(paParams);
	return DisplayEnumerator().Enumerate(window);
}

std::wstring ScreenManager::GetDisplayInfo(tVariant* paParams, const long lSizeArray)
{
	Window window = 0;
	if (lSizeArray > 0) window = VarToInt(paParams);
	return DisplayFinder().FindDisplay(window);
}

BOOL ScreenManager::CaptureScreen(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	return CaptureWindow(pvarRetValue, 0);
}

BOOL ScreenManager::CaptureWindow(tVariant* pvarRetValue, HWND hWnd)
{
	Display* display = XOpenDisplay(NULL);
	if (display == NULL) return false;

	Window window = hWnd;
	if (window == 0) window = DefaultRootWindow(display);

	BOOL success = false;
	XWindowAttributes gwa;
	XGetWindowAttributes(display, window, &gwa);
	XImage* image = XGetImage(display, window, 0, 0, gwa.width, gwa.height, AllPlanes, ZPixmap);
	X11Screenshot screenshot = X11Screenshot(image);
	std::vector<char> buffer;
	if (screenshot.save_to_png(buffer)) {
		pvarRetValue->strLen = buffer.size();
		m_addin->AllocMemory((void**)&pvarRetValue->pstrVal, pvarRetValue->strLen);
		TV_VT(pvarRetValue) = VTYPE_BLOB;
		if (pvarRetValue->pstrVal) {
			memcpy((void*)pvarRetValue->pstrVal, &buffer[0], pvarRetValue->strLen);
			success = true;
		}
	}
	XDestroyImage(image);
	XCloseDisplay(display);
	return success;
}

class ProcWindows : public WindowEnumerator
{
private:
	const unsigned long m_pid = 0;
	std::map<Window, bool> m_map;
protected:
	virtual bool EnumWindow(Window window) {
		unsigned long pid = GetWindowPid(window);
		if (m_pid == pid) {
			if (m_map.find(window) == m_map.end()) m_map[window] = true;
			Window parent = GetWindowOwner(window);
			if (parent) m_map[parent] = false;
		}
		return true;
	}
public:
	ProcWindows(unsigned long pid)
		: WindowEnumerator(), m_pid(pid) {}

	static Window TopWindow(unsigned long pid) {
		ProcWindows p(pid);
		p.Enumerate();
		for (auto it = p.m_map.begin(); it != p.m_map.end(); it++) {
			if (it->second) return it->first;
		}
		return 0;
	}
};

BOOL ScreenManager::CaptureProcess(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray <= 0) return false;
	Window window = ProcWindows::TopWindow(VarToInt(paParams));
	if (window) return CaptureWindow(pvarRetValue, window);
	return true;
}

class ScreenHelper : public WindowHelper
{
public:
	ScreenHelper() {
		unsigned long* geometry = NULL;
		unsigned long geometry_size = 0;
		unsigned long* workarea = NULL;
		unsigned long workarea_size = 0;
		Window root = DefaultRootWindow(display);
		if (!GetProperty(root, XA_CARDINAL, "_NET_DESKTOP_GEOMETRY", VXX(&geometry), &geometry_size)) return;
		if (!GetProperty(root, XA_CARDINAL, "_NET_WORKAREA", VXX(&workarea), &workarea_size)) {
			if (!GetProperty(root, XA_CARDINAL, "_WIN_WORKAREA", VXX(&workarea), &workarea_size)) {
				XFree(geometry);
				return;
			}
		};
		if (geometry_size >= 2) {
			json["Left"] = json["Top"] = 0;
			json["Right"] = json["Width"] = geometry[0];
			json["Bottom"] = json["Height"] = geometry[1];
		}
		if (workarea_size >= 4) {
			JSON j;
			j["Left"] = workarea[0];
			j["Top"] = workarea[1];
			j["Width"] = workarea[2];
			j["Height"] = workarea[3];
			json["Work"] = j;
		}
		XFree(geometry);
		XFree(workarea);
	}
};

std::wstring ScreenManager::GetScreenInfo()
{
	ScreenHelper helper;
	if (helper) return helper;

	JSON json;
	Display* display = XOpenDisplay(NULL);
	if (!display) return {};
	int number = DefaultScreen(display);
	json["Left"] = json["Top"] = 0;
	json["Right"] = json["Width"] = DisplayWidth(display, number);
	json["Bottom"] = json["Height"] = DisplayHeight(display, number);
	XCloseDisplay(display);
	return json;
}

std::wstring ScreenManager::GetCursorPos()
{
	Display* dsp = XOpenDisplay(NULL);
	if (!dsp) return {};

	int screenNumber = DefaultScreen(dsp);

	XEvent event;

	/* get info about current pointer position */
	XQueryPointer(dsp, RootWindow(dsp, DefaultScreen(dsp)),
		&event.xbutton.root, &event.xbutton.window,
		&event.xbutton.x_root, &event.xbutton.y_root,
		&event.xbutton.x, &event.xbutton.y,
		&event.xbutton.state
	);

	XCloseDisplay(dsp);

	JSON json;
	json["x"] = event.xbutton.x;
	json["y"] = event.xbutton.y;
	return json;
}

BOOL ScreenManager::SetCursorPos(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 2) return false;
	const int x = VarToInt(paParams);
	const int y = VarToInt(paParams + 1);
	Display* display = XOpenDisplay(0);
	Window root_window = XRootWindow(display, 0);
	XSelectInput(display, root_window, KeyReleaseMask);
	XWarpPointer(display, None, root_window, 0, 0, 0, 0, x, y);
	XFlush(display);
	XCloseDisplay(display);
	return true;
}

class Hotkey
	: private std::vector<KeyCode>
{
private:
	Display* m_display;
public:
	Hotkey() {
		m_display = XOpenDisplay(NULL);
	}
	virtual ~Hotkey() {
		if (m_display) XCloseDisplay(m_display);
	}
	bool add(const std::wstring &text) {
		try {
			auto json = JSON::parse(WC2MB(text));
			for (JSON::iterator it = json.begin(); it != json.end(); ++it) {
				add((KeySym)it.value());
			}
		}
		catch (nlohmann::json::parse_error e) {
			return false;
		}
		return true;
	}
	void add(KeySym keysym) {
		KeyCode keycode = XKeysymToKeycode(m_display, keysym);
		std::wcout << std::endl << keysym << " : " << keycode << std::endl;
		push_back(keycode);
	}
	void send() {
		if (!m_display) return;
		if (size() == 0) return;
		for (auto it = begin(); it != end(); ++it) {
			XTestFakeKeyEvent(m_display, *it, true, CurrentTime);
		}
		std::reverse(begin(), end());
		for (auto it = begin(); it != end(); ++it) {
			XTestFakeKeyEvent(m_display, *it, false, CurrentTime);
		}
		XFlush(m_display);
	}
	void down() {
		if (!m_display) return;
		if (size() == 0) return;
		for (auto it = begin(); it != end(); ++it) {
			XTestFakeKeyEvent(m_display, *it, true, CurrentTime);
		}
	}
	void up() {
		if (!m_display) return;
		if (size() == 0) return;
		std::reverse(begin(), end());
		for (auto it = begin(); it != end(); ++it) {
			XTestFakeKeyEvent(m_display, *it, false, CurrentTime);
		}
	}
};

BOOL ScreenManager::EmulateClick(tVariant* paParams, const long lSizeArray)
{
	Display* display = XOpenDisplay(NULL);
	if (!display) return false;

	unsigned int button = 1;
	switch (VarToInt(paParams)) {
	case 1: button = 3; break;
	case 2: button = 2; break;
	}

	Hotkey hotkey;
	tVariant* pvarKeys = paParams + 1;
	if (TV_VT(pvarKeys) == VTYPE_PWSTR) {
		if (pvarKeys->pwstrVal == nullptr) return false;
		if (!hotkey.add(VarToStr(pvarKeys))) return false;
	}
	else if (TV_VT(pvarKeys) == VTYPE_I4) {
		WORD flags = VarToInt(pvarKeys);
		if (flags & 0x04) hotkey.add(XK_Shift_L);
		if (flags & 0x08) hotkey.add(XK_Control_L);
		if (flags & 0x10) hotkey.add(XK_Alt_L);
	}
	hotkey.down();
	XTestFakeButtonEvent(display, button, true, CurrentTime);
	XTestFakeButtonEvent(display, button, false, CurrentTime);
	hotkey.up();
	XFlush(display);
	XCloseDisplay(display);
	return true;
}

BOOL ScreenManager::EmulateWheel(tVariant* paParams, const long lSizeArray)
{
	Display* display = XOpenDisplay(NULL);
	if (!display) return false;
	unsigned int button = VarToInt(paParams) < 0 ? 5 : 4;

	Hotkey hotkey;
	tVariant* pvarKeys = paParams + 1;
	if (TV_VT(pvarKeys) == VTYPE_PWSTR) {
		if (pvarKeys->pwstrVal == nullptr) return false;
		if (!hotkey.add(VarToStr(pvarKeys))) return false;
	}
	else if (TV_VT(pvarKeys) == VTYPE_I4) {
		WORD flags = VarToInt(pvarKeys);
		if (flags & 0x04) hotkey.add(XK_Shift_L);
		if (flags & 0x08) hotkey.add(XK_Control_L);
		if (flags & 0x10) hotkey.add(XK_Alt_L);
	}
	hotkey.down();
	XTestFakeButtonEvent(display, button, true, CurrentTime);
	XTestFakeButtonEvent(display, button, false, CurrentTime);
	hotkey.up();
	XFlush(display);
	XCloseDisplay(display);
	return true;
}

BOOL ScreenManager::EmulateDblClick(tVariant* paParams, const long lSizeArray)
{
	Display* display = XOpenDisplay(NULL);
	if (!display) return false;
	XTestFakeButtonEvent(display, 1, true, CurrentTime);
	XTestFakeButtonEvent(display, 1, false, CurrentTime);
	XTestFakeButtonEvent(display, 1, true, CurrentTime);
	XTestFakeButtonEvent(display, 1, false, CurrentTime);
	XFlush(display);
	XCloseDisplay(display);
	return true;
}

BOOL ScreenManager::EmulateMouse(tVariant* paParams, const long lSizeArray)
{
	Display* display = XOpenDisplay(NULL);
	if (!display) return false;

	double x2 = VarToInt(paParams);
	double y2 = VarToInt(paParams + 1);
	double count = VarToInt(paParams + 2);
	DWORD pause = VarToInt(paParams + 3);

	XEvent event;
	XQueryPointer(display, RootWindow(display, DefaultScreen(display)),
		&event.xbutton.root, &event.xbutton.window,
		&event.xbutton.x_root, &event.xbutton.y_root,
		&event.xbutton.x, &event.xbutton.y,
		&event.xbutton.state
	);
	double x1 = event.xbutton.x;
	double y1 = event.xbutton.y;

	double dx = x2 - x1;
	double dy = y2 - y1;
	int px = 3, py = 2;
	if (abs(dx) < abs(dy)) {
		px = 2; py = 3;
	}
	count /= 2;
	double cx = pow(count, px) * 2;
	double cy = pow(count, py) * 2;

	for (double i = 1; i <= count; i++) {
		int xx = round(x1 + dx * pow(i, px) / cx);
		int yy = round(y1 + dy * pow(i, py) / cy);
		XTestFakeMotionEvent(display, DefaultScreen(display), xx, yy, CurrentTime);
		XFlush(display);
		if (pause) usleep(pause * 1000);
	}
	for (double i = count - 1; i >= 0; i--) {
		int xx = round(x2 - dx * pow(i, px) / cx);
		int yy = round(y2 - dy * pow(i, py) / cy);
		XTestFakeMotionEvent(display, DefaultScreen(display), xx, yy, CurrentTime);
		XFlush(display);
		if (pause) usleep(pause * 1000);
	}

	XCloseDisplay(display);

	return true;
}

BOOL ScreenManager::EmulateHotkey(tVariant* paParams, const long lSizeArray)
{
	usleep(100 * 1000);
	Hotkey hotkey;
	if (TV_VT(paParams) == VTYPE_PWSTR) {
		if (paParams->pwstrVal == nullptr) return false;
		if (!hotkey.add(VarToStr(paParams))) return false;
	}
	else {
		auto key = VarToInt(paParams);
		auto flags = VarToInt(paParams + 1);
		if (flags & 0x04) hotkey.add(XK_Shift_L);
		if (flags & 0x08) hotkey.add(XK_Control_L);
		if (flags & 0x10) hotkey.add(XK_Alt_L);
		hotkey.add(key);
	}
	hotkey.send();
	return true;
}

BOOL ScreenManager::EmulateText(tVariant* paParams, const long lSizeArray)
{
	std::wcout << L"EmulateText";
	Display* display = XOpenDisplay(NULL);
	if (!display) return false;
	usleep(100 * 1000);
	std::wstring text = VarToStr(paParams);
	auto pause = VarToInt(paParams + 1);
	for (auto ch : text) {
		std::wstring w; w += ch;
		KeySym keysym = XStringToKeysym(WC2MB(w).data());
		KeyCode keycode = XKeysymToKeycode(display, keysym);
		std::wcout << std::endl << w << " : " << keysym << " : " << keycode << std::endl;
		XTestFakeKeyEvent(display, keycode, true, CurrentTime);
		XTestFakeKeyEvent(display, keycode, false, CurrentTime);
		XFlush(display);
		if (pause) usleep(pause * 1000);
	}
	XCloseDisplay(display);
	return true;
}

#endif //_WINDOWS
