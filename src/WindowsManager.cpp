#include "stdafx.h"
#include "WindowsManager.h"

#ifdef _WINDOWS

static JSON WindowInfo(HWND hWnd, DWORD dwProcessId = 0)
{
	if (dwProcessId == 0) ::GetWindowThreadProcessId(hWnd, &dwProcessId);

	JSON json;
	json["Window"] = (int64_t)hWnd;
	json["Enabled"] = (boolean)::IsWindowEnabled(hWnd);
	json["Owner"] = (int64_t)::GetWindow(hWnd, GW_OWNER);
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
	class Param {
	public:
		DWORD pid = 0;
		JSON json;
	};
	Param param;
	param.pid = (DWORD)pid;
	bool bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			if (::IsWindow(hWnd) && ::IsWindowVisible(hWnd)) {
				Param* p = (Param*)lParam;
				DWORD dwProcessId;
				::GetWindowThreadProcessId(hWnd, &dwProcessId);
				if (p->pid == 0 || p->pid == dwProcessId) {
					JSON j = WindowInfo(hWnd, dwProcessId);
					p->json.push_back(j);
				}
			}
			return TRUE;
		}, (LPARAM)&param);

	return param.json.dump();
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
	json["Window"] = (INT64)hWnd;
	return json.dump();
}

HWND WindowsManager::ActiveWindow()
{
	return ::GetForegroundWindow();
}

HWND WindowsManager::CurrentWindow()
{
	DWORD pid = GetCurrentProcessId();
	std::pair<HWND, DWORD> params = { 0, pid };

	// Enumerate the windows using a lambda to process each window
	bool bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			auto pParams = (std::pair<HWND, DWORD>*)(lParam);
			WCHAR buffer[256];
			DWORD processId;
			if (IsWindowVisible(hWnd)
				&& ::GetWindowThreadProcessId(hWnd, &processId)
				&& processId == pParams->second
				&& ::GetClassName(hWnd, buffer, 256)
				&& wcscmp(L"V8TopLevelFrameSDI", buffer) == 0
				) {
				// Stop enumerating
				SetLastError(-1);
				pParams->first = hWnd;
				return FALSE;
			}

			// Continue enumerating
			return TRUE;
		}, (LPARAM)&params);

	if (!bResult && GetLastError() == -1 && params.first)
	{
		return params.first;
	}

	return 0;
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
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (IsWindow(hWnd)) {
		if (IsWindowVisible(hWnd)) {
			WINDOWPLACEMENT place;
			memset(&place, 0, sizeof(WINDOWPLACEMENT));
			place.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hWnd, &place);
			if (place.showCmd == SW_SHOWMINIMIZED) ShowWindow(hWnd, SW_RESTORE);
			SetForegroundWindow(hWnd);
		}
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
			j["Window"] = window;
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

class WindowInfo : public WindowHelper
{
public:
	WindowInfo(Window window) {
		json["Window"] = window;
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
		json["Window"] = window;
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
