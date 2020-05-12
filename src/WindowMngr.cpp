#include "stdafx.h"
#include "WindowMngr.h"
#include "json_ext.h"

#ifdef _WINDOWS

static JSON WindowInfo(HWND hWnd, DWORD dwProcessId = 0)
{
	if (dwProcessId == 0) ::GetWindowThreadProcessId(hWnd, &dwProcessId);

	JSON json;
	json["Window"] = (INT64)hWnd;
	json["Enabled"] = (boolean)::IsWindowEnabled(hWnd);
	json["Owner"] = (INT64)::GetWindow(hWnd, GW_OWNER);
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

std::wstring WindowManager::GetWindowList(tVariant* paParams, const long lSizeArray)
{
	class Param {
	public:
		DWORD pid = 0;
		JSON json;
	};
	Param param;

	if (lSizeArray > 0) param.pid = VarToInt(paParams);

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

	return param.json;
}

std::wstring WindowManager::GetWindowInfo(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return {};
	JSON json = WindowInfo(hWnd);
	json["Maximized"] = IsMaximized(hWnd);
	return json;
}

std::wstring WindowManager::GetWindowSize(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	RECT rect{0,0,0,0};
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return {};
	::GetWindowRect(hWnd, &rect);
	JSON json;
	json["Left"] = rect.left;
	json["Top"] = rect.top;
	json["Right"] = rect.right;
	json["Bottom"] = rect.bottom;
	json["Width"] = rect.right - rect.left;
	json["Height"] = rect.bottom - rect.top;
	json["Window"] = (INT64)hWnd;
	return json;
}

HWND WindowManager::ActiveWindow()
{
	return ::GetForegroundWindow();
}

HWND WindowManager::CurrentWindow()
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

bool WindowManager::SetWindowSize(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 3) return false;
	HWND hWnd = VarToHwnd(paParams);
	int w = VarToInt(paParams + 1);
	int h = VarToInt(paParams + 2);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	::SetWindowPos(hWnd, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	::UpdateWindow(hWnd);
	return true;
}

bool WindowManager::SetWindowPos(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 5) return false;
	HWND hWnd = VarToHwnd(paParams);
	int x = VarToInt(paParams + 1);
	int y = VarToInt(paParams + 2);
	int w = VarToInt(paParams + 3);
	int h = VarToInt(paParams + 4);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	UINT uFlags = SWP_NOZORDER | SWP_NOOWNERZORDER;
	if (w == 0 && h == 0) uFlags |= SWP_NOSIZE;
	::SetWindowPos(hWnd, 0, x, y, w, h, uFlags);
	::UpdateWindow(hWnd);
	return true;
}

bool WindowManager::EnableResizing(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 2) return false;
	HWND hWnd = VarToHwnd(paParams);
	bool enable = VarToInt(paParams + 1);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	LONG style = ::GetWindowLong(hWnd, GWL_STYLE);
	style = enable ? (style | WS_SIZEBOX) : (style & ~WS_SIZEBOX);
	::SetWindowLong(hWnd, GWL_STYLE, style);
	::UpdateWindow(hWnd);
	return true;
}

bool WindowManager::Minimize(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	return SetWindowState(hWnd, 0, true);
}

bool WindowManager::Restore(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	return SetWindowState(hWnd, 1, true);
}

bool WindowManager::Maximize(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	return SetWindowState(hWnd, 2, true);
}

bool WindowManager::Activate(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return false;
	HWND hWnd = VarToHwnd(paParams);
	if (!IsWindow(hWnd)) return true;

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

bool WindowManager::IsMaximized(HWND hWnd)
{
	WINDOWPLACEMENT place;
	memset(&place, 0, sizeof(WINDOWPLACEMENT));
	place.length = sizeof(WINDOWPLACEMENT);
	if (!GetWindowPlacement(hWnd, &place)) return false;
	return place.showCmd == SW_SHOWMAXIMIZED;
}

int32_t WindowManager::GetWindowState(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
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

bool WindowManager::SetWindowState(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	int iMode = 1;
	bool bActivate = true;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
	if (lSizeArray > 1) iMode = (paParams + 1)->intVal;
	if (lSizeArray > 2) bActivate = (paParams + 2)->bVal;
	if (hWnd == 0) hWnd = ::GetForegroundWindow();
	if (!IsWindow(hWnd)) return true;
	return SetWindowState(hWnd, iMode, bActivate);
}

bool WindowManager::SetWindowState(HWND hWnd, int iMode, bool bActivate)
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

bool WindowManager::IsMaximized(HWND hWnd)
{
	//TODO
	return false;
}

std::wstring WindowManager::GetWindowList(tVariant* paParams, const long lSizeArray)
{
	unsigned long pid = 0;
	if (lSizeArray > 0) pid = VarToInt(paParams);
	return WindowList(pid).Enumerate();
}

std::wstring WindowManager::GetWindowInfo(tVariant* paParams, const long lSizeArray)
{
	Window window = 0;
	if (lSizeArray > 0) window = VarToInt(paParams);
	if (window == 0) window = ActiveWindow();
	return WindowInfo(window);
}

std::wstring WindowManager::GetWindowSize(tVariant* paParams, const long lSizeArray)
{
	Window window = 0;
	if (lSizeArray > 0) window = VarToInt(paParams);
	if (window == 0) window = ActiveWindow();
	return WindowSize(window);
}

HWND WindowManager::ActiveWindow()
{
	return WindowHelper().GetActiveWindow();
}

bool WindowManager::Activate(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return false;
	Window window = VarToInt(paParams);
	WindowHelper().SetActiveWindow(window);
	return true;
}

bool  WindowManager::Restore(tVariant* paParams, const long lSizeArray)
{
	Window window = 0;
	if (lSizeArray > 0) window = VarToInt(paParams);
	if (window == 0) window = ActiveWindow();
	WindowHelper().Maximize(window, false);
	return true;
}

bool  WindowManager::Maximize(tVariant* paParams, const long lSizeArray)
{
	Window window = 0;
	if (lSizeArray > 0) window = VarToInt(paParams);
	if (window == 0) window = ActiveWindow();
	WindowHelper().Maximize(window, true);
	return true;
}

bool WindowManager::Minimize(tVariant* paParams, const long lSizeArray)
{
	Window window = 0;
	if (lSizeArray > 0) window = VarToInt(paParams);
	if (window == 0) window = ActiveWindow();
	WindowHelper().Minimize(window);
	return true;
}

bool WindowManager::SetWindowSize(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 3) return false;
	Window window = VarToInt(paParams);
	if (window == 0) window = ActiveWindow();
	int x = VarToInt(paParams + 1);
	int y = VarToInt(paParams + 2);
	WindowHelper().SetWindowSize(window, x, y);
	return true;
}

bool WindowManager::SetWindowPos(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 5) return false;
	Window window = VarToInt(paParams);
	if (window == 0) window = ActiveWindow();
	int x = VarToInt(paParams + 1);
	int y = VarToInt(paParams + 2);
	int w = VarToInt(paParams + 3);
	int h = VarToInt(paParams + 4);
	WindowHelper().SetWindowPos(window, x, y, w, h);
	return true;
}

bool WindowManager::EnableResizing(tVariant* paParams, const long lSizeArray)
{
	return true;
}

#endif //_WINDOWS
