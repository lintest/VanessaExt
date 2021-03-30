#include "stdafx.h"
#include "WindowsManager.h"

int64_t WindowsManager::ActivateProcess(int64_t pid)
{
	int64_t window = GetTopProcessWindow(pid);
	return window && Activate(window);
}

#ifdef _WINDOWS

#pragma warning (disable : 4244)

static JSON WindowInfo(HWND hWnd, DWORD dwProcessId = 0)
{
	if (dwProcessId == 0) ::GetWindowThreadProcessId(hWnd, &dwProcessId);

	JSON json;
	json["Window"] = (uint64_t)hWnd;
	json["Enabled"] = (boolean)::IsWindowEnabled(hWnd);
	json["Owner"] = (uint64_t)::GetWindow(hWnd, GW_OWNER);
	json["ProcessId"] = dwProcessId;

	WCHAR buffer[256];
	::GetClassName(hWnd, buffer, 256);
	json["Class"] = WC2MB(buffer);

	const int length = GetWindowTextLength(hWnd);
	if (length != 0) {
		std::wstring text;
		text.resize(length);
		::GetWindowText(hWnd, &text[0], length + 1);
		json["Title"] = WC2MB(text);
	}
	return json;
}

std::string WindowsManager::GetWindowList(int64_t pid)
{
	using EnumParam = std::pair<DWORD, JSON>;
	EnumParam p{ (DWORD)pid, {} };
	bool bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			if (::IsWindow(hWnd)) {
				auto p = (EnumParam*)lParam;
				DWORD dwProcessId;
				::GetWindowThreadProcessId(hWnd, &dwProcessId);
				if (p->first == 0 || p->first == dwProcessId) {
					JSON j = WindowInfo(hWnd, dwProcessId);
					p->second.push_back(j);
				}
			}
			return TRUE;
		}, (LPARAM)&p);

	return p.second.dump();
}

std::string WindowsManager::GetWindowInfo(int64_t window)
{
	HWND hWnd = (HWND)window;
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return {};
	JSON json = WindowInfo(hWnd);
	json["Maximized"] = IsMaximized(hWnd);
	return json.dump();
}

std::string WindowsManager::GetWindowSize(int64_t window)
{
	HWND hWnd = (HWND)window;
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return {};
	RECT rect{ 0,0,0,0 };
	::GetWindowRect(hWnd, &rect);
	JSON json;
	json["Left"] = rect.left;
	json["Top"] = rect.top;
	json["Right"] = rect.right;
	json["Bottom"] = rect.bottom;
	json["Width"] = rect.right - rect.left;
	json["Height"] = rect.bottom - rect.top;
	json["Window"] = (uint64_t)hWnd;
	return json.dump();
}

int64_t WindowsManager::ActiveWindow()
{
	return (int64_t)::GetForegroundWindow();
}

int64_t WindowsManager::GetTopProcessWindow(int64_t pid)
{
	using EnumParam = std::pair<DWORD, std::map<HWND, bool>>;
	EnumParam p{ (DWORD)pid, {} };
	bool bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			if (::IsWindow(hWnd)) {
				auto p = (EnumParam*)lParam;
				DWORD dwProcessId;
				WCHAR buffer[256];
				::GetWindowThreadProcessId(hWnd, &dwProcessId);
				if (p->first == dwProcessId
					&& ::GetClassName(hWnd, buffer, 256)
					&& (wcscmp(L"V8TopLevelFrameSDIsec", buffer) == 0
						|| wcscmp(L"V8TopLevelFrameSDI", buffer) == 0
						|| wcscmp(L"V8TopLevelFrame", buffer) == 0)
					) {
					auto& map = p->second;
					if (map.find(hWnd) == map.end()) map[hWnd] = true;
					HWND hParent = ::GetWindow(hWnd, GW_OWNER);
					if (hParent) map[hParent] = false;
				}
			}
			return TRUE;
		}, (LPARAM)&p);
	for (auto it = p.second.begin(); it != p.second.end(); it++) {
		if (it->second) return (int64_t)it->first;
	}
	return 0;
}

int64_t WindowsManager::GetMainProcessWindow(int64_t pid)
{
	using EnumParam = std::pair<DWORD, HWND>;
	EnumParam p{ (DWORD)pid, 0 };
	bool bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			if (::IsWindow(hWnd)) {
				auto p = (EnumParam*)lParam;
				DWORD dwProcessId;
				WCHAR buffer[256];
				::GetWindowThreadProcessId(hWnd, &dwProcessId);
				if (p->first == dwProcessId
					&& ::GetClassName(hWnd, buffer, 256)
					&& wcscmp(L"V8TopLevelFrameSDI", buffer) == 0
					) {
					p->second = hWnd;
				}
			}
			return TRUE;
		}, (LPARAM)&p);
	return (int64_t)p.second;
}

bool WindowsManager::SetWindowSize(int64_t window, int64_t w, int64_t h)
{
	HWND hWnd = HWND(window);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	::SetWindowPos(hWnd, 0, 0, 0, (int)w, (int)h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	::UpdateWindow(hWnd);
	return true;
}

bool WindowsManager::SetWindowPos(int64_t window, int64_t x, int64_t y, int64_t w, int64_t h)
{
	HWND hWnd = HWND(window);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	UINT uFlags = SWP_NOZORDER | SWP_NOOWNERZORDER;
	if (w == 0 && h == 0) uFlags |= SWP_NOSIZE;
	::SetWindowPos(hWnd, 0, (int)x, (int)y, (int)w, (int)h, uFlags);
	::UpdateWindow(hWnd);
	return true;
}

bool WindowsManager::EnableResizing(int64_t window, bool enable)
{
	HWND hWnd = HWND(window);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	LONG style = ::GetWindowLong(hWnd, GWL_STYLE);
	style = enable ? (style | WS_SIZEBOX) : (style & ~WS_SIZEBOX);
	::SetWindowLong(hWnd, GWL_STYLE, style);
	::UpdateWindow(hWnd);
	return true;
}

bool WindowsManager::Minimize(int64_t window)
{
	HWND hWnd = HWND(window);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	return SetWindowState(hWnd, 0, true);
}

bool WindowsManager::Restore(int64_t window)
{
	HWND hWnd = HWND(window);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	return SetWindowState(hWnd, 1, true);
}

bool WindowsManager::Maximize(int64_t window)
{
	HWND hWnd = HWND(window);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	return SetWindowState(hWnd, 2, true);
}

bool WindowsManager::Activate(int64_t window)
{
	HWND hWnd = HWND(window);
	if (hWnd && IsWindow(hWnd) && IsWindowVisible(hWnd)) {
		if (IsIconic(hWnd)) ShowWindow(hWnd, SW_RESTORE);
		keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY, 0);
		keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		SetForegroundWindow(hWnd);
	}
	return true;
}

bool WindowsManager::IsMaximized(HWND hWnd)
{
	WINDOWPLACEMENT place;
	memset(&place, 0, sizeof(WINDOWPLACEMENT));
	place.length = sizeof(WINDOWPLACEMENT);
	if (!GetWindowPlacement(hWnd, &place)) return false;
	return place.showCmd == SW_SHOWMAXIMIZED;
}

int64_t WindowsManager::GetWindowState(int64_t window)
{
	HWND hWnd = HWND(window);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return 0;

	WINDOWPLACEMENT place;
	memset(&place, 0, sizeof(WINDOWPLACEMENT));
	place.length = sizeof(WINDOWPLACEMENT);
	if (!GetWindowPlacement(hWnd, &place)) return -1;

	switch (place.showCmd) {
	case SW_SHOWMINIMIZED:
		return 0;
	case SW_SHOWNORMAL:
		return 1;
	case SW_SHOWMAXIMIZED:
		return 2;
	default:
		return -1;
	}
}

bool WindowsManager::SetWindowState(int64_t window, int64_t mode, bool activate)
{
	HWND hWnd = HWND(window);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	return SetWindowState(hWnd, (int)mode, activate);
}

bool WindowsManager::SetWindowState(HWND hWnd, int iMode, bool bActivate)
{
	if (IsWindow(hWnd)) {
		if (IsWindowVisible(hWnd)) {
			int nCmdShow;
			if (bActivate) {
				switch (iMode) {
				case 0: nCmdShow = SW_SHOWMINIMIZED; break;
				case 2: nCmdShow = SW_SHOWMAXIMIZED; break;
				default: nCmdShow = SW_RESTORE;
				}
			}
			else {
				switch (iMode) {
				case 0: nCmdShow = SW_SHOWMINNOACTIVE; break;
				case 2: nCmdShow = SW_MAXIMIZE; break;
				default: nCmdShow = SW_RESTORE;
				}
			}
			::ShowWindow(hWnd, nCmdShow);
		}
	}
	return true;
}

int64_t WindowsManager::FindWindow(const std::wstring& name, const std::wstring& title)
{
	LPCWSTR lpClassName = nullptr;
	LPCWSTR lpWindowName = nullptr;
	if (!name.empty()) lpClassName = name.c_str();
	if (!title.empty()) lpWindowName = title.c_str();
	return (int64_t)::FindWindow(lpClassName, lpWindowName);
}

bool WindowsManager::PostMessage(int64_t hWnd, int64_t Msg, int64_t wParam, int64_t lParam)
{
	return ::PostMessage(HWND(hWnd), Msg, wParam, lParam);
}

#else //_WINDOWS

#include "XWinBase.h"

class WindowList : public WindowEnumerator
{
private:
	const unsigned long m_pid = 0;
protected:
	virtual bool EnumWindow(Window window) {
		unsigned long pid = GetWindowPid(window);
		if (m_pid == 0 || m_pid == pid) {
			JSON j;
			j["Window"] = (uint64_t)window;
			j["Owner"] = GetWindowOwner(window);
			j["Class"] = GetWindowClass(window);
			j["Title"] = GetWindowTitle(window);
			j["ProcessId"] = pid;
			json.push_back(j);
		}
		return true;
	}
public:
	WindowList(unsigned long pid = 0)
		: WindowEnumerator(), m_pid(pid) {}
};

class TopWindows : public WindowEnumerator
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
	TopWindows(unsigned long pid)
		: WindowEnumerator(), m_pid(pid) {}

	static Window find(unsigned long pid) {
		TopWindows p(pid);
		p.Enumerate();
		for (auto it = p.m_map.begin(); it != p.m_map.end(); it++) {
			if (it->second) return it->first;
		}
		return 0;
	}
};

class MainWindows : public WindowEnumerator
{
private:
	const unsigned long m_pid = 0;
	Window m_window = 0;
protected:
	virtual bool EnumWindow(Window window) {
		unsigned long pid = GetWindowPid(window);
		if (m_pid == pid) {
			Window parent = GetWindowOwner(window);
			if (!parent) m_window = window;
		}
		return true;
	}
public:
	MainWindows(unsigned long pid)
		: WindowEnumerator(), m_pid(pid) {}

	static Window find(unsigned long pid) {
		MainWindows p(pid);
		p.Enumerate();
		return p.m_window;
	}
};

class WindowInfo : public WindowHelper
{
public:
	WindowInfo(Window window) {
		json["Window"] = (uint64_t)window;
		json["Owner"] = GetWindowOwner(window);
		json["Class"] = GetWindowClass(window);
		json["Title"] = GetWindowTitle(window);
		json["Maximized"] = IsMaximized(window);
		json["ProcessId"] = GetWindowPid(window);
	}
};

class WindowSize : public WindowHelper
{
public:
	WindowSize(Window window) {
		Window junkroot;
		int x, y, junkx, junky;
		unsigned int w, h, bw, depth;
		Status status = XGetGeometry(display, window, &junkroot, &junkx, &junky, &w, &h, &bw, &depth);
		if (!status) return;
		Bool ok = XTranslateCoordinates(display, window, junkroot, junkx, junky, &x, &y, &junkroot);
		if (!ok) return;
		json["Left"] = x;
		json["Top"] = y;
		json["Width"] = w;
		json["Height"] = h;
		json["Right"] = x + w;
		json["Bottom"] = y + h;
		json["Window"] = (uint64_t)window;
	}
};

bool WindowsManager::IsMaximized(int64_t window)
{
	//TODO
	return false;
}

std::string WindowsManager::GetWindowList(int64_t pid)
{
	return WindowList(pid).Enumerate();
}

int64_t WindowsManager::GetMainProcessWindow(int64_t pid)
{
	return (int64_t)MainWindows::find(pid);
}

int64_t WindowsManager::GetTopProcessWindow(int64_t pid)
{
	return (int64_t)TopWindows::find(pid);
}

std::string WindowsManager::GetWindowInfo(int64_t window)
{
	if (window == 0) window = ActiveWindow();
	return WindowInfo(window);
}

std::string WindowsManager::GetWindowSize(int64_t window)
{
	if (window == 0) window = ActiveWindow();
	return WindowSize(window);
}

int64_t WindowsManager::ActiveWindow()
{
	return WindowHelper().GetActiveWindow();
}

bool WindowsManager::Activate(int64_t window)
{
	WindowHelper().SetActiveWindow(window);
	return true;
}

bool  WindowsManager::Restore(int64_t window)
{
	if (window == 0) window = ActiveWindow();
	WindowHelper().Maximize(window, false);
	return true;
}

bool  WindowsManager::Maximize(int64_t window)
{
	if (window == 0) window = ActiveWindow();
	WindowHelper().Maximize(window, true);
	return true;
}

bool WindowsManager::Minimize(int64_t window)
{
	if (window == 0) window = ActiveWindow();
	WindowHelper().Minimize(window);
	return true;
}

bool WindowsManager::SetWindowSize(int64_t window, int64_t w, int64_t h)
{
	if (window == 0) window = ActiveWindow();
	WindowHelper().SetWindowSize(window, w, h);
	return true;
}

bool WindowsManager::SetWindowPos(int64_t window, int64_t x, int64_t y, int64_t w, int64_t h)
{
	if (window == 0) window = ActiveWindow();
	WindowHelper().SetWindowPos(window, x, y, w, h);
	return true;
}

bool WindowsManager::EnableResizing(int64_t window, bool enable)
{
	//TODO
	return true;
}

#endif //_WINDOWS
