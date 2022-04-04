#ifdef _WINDOWS

#include "EventMonitor.h"

#include "AddInNative.h"
#include "Windowsx.h"

namespace EventMonitor {

	std::string msg2str(WPARAM message)
	{
		switch (message) {
		case WM_LBUTTONDOWN:
			return "LBUTTONDOWN";
		case WM_LBUTTONUP:
			return "LBUTTONUP";
		case WM_LBUTTONDBLCLK:
			return "LBUTTONDBLCLK";
		case WM_RBUTTONDOWN:
			return "RBUTTONDOWN";
		case WM_RBUTTONUP:
			return "RBUTTONUP";
		case WM_RBUTTONDBLCLK:
			return "RBUTTONDBLCLK";
		default:
			return {};
		}
	}

	class Hooker {
	private:
		AddInNative& addin;
	public:
		Hooker(AddInNative* addin) : addin(*addin) {}
		LRESULT onMouseEvent(WPARAM wParam, LPARAM lParam);
		void create();
	};

	Hooker* getHooker(HWND hWnd)
	{
		return (Hooker*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}

	Hooker* getHooker(LPVOID lpParam)
	{
		return (Hooker*)lpParam;
	}

	LRESULT Hooker::onMouseEvent(WPARAM wParam, LPARAM lParam) {
		POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		JSON json = {
			{ "event", msg2str(wParam) },
			{ "x", GET_X_LPARAM(lParam) },
			{ "y", GET_Y_LPARAM(lParam) },
		};
		std::u16string text = MB2WCHAR(json.dump());
		addin.ExternalEvent(u"MOUSEHOOK", text);
		return 0L;
	}

	const UINT WM_MOUSE_HOOK = WM_USER + 10;

	LRESULT CALLBACK HookerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_MOUSE_HOOK:
			return getHooker(hWnd)->onMouseEvent(wParam, lParam);
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	static DWORD WINAPI HookerThreadProc(LPVOID lpParam)
	{
		auto hooker = getHooker(lpParam);
		hooker->create();
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		delete hooker;
		return 0;
	}

	const LPCWSTR wsHookerName = L"VanessaUserAutoHooker";

	static HHOOK hMouseHook = NULL;

	void onMouseEvent(WPARAM wParam, LPARAM lParam)
	{
		auto hWnd = FindWindow(wsHookerName, NULL);
		if (hWnd == NULL) return;
		auto pStruct = (PMOUSEHOOKSTRUCT)lParam;
		POINT pt = pStruct->pt;
		ClientToScreen(pStruct->hwnd, &pt);
		SendMessage(hWnd, WM_MOUSE_HOOK, wParam, MAKELPARAM(pt.x, pt.y));
	}

	LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION) {
			switch (wParam) {
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_RBUTTONDBLCLK:
				onMouseEvent(wParam, lParam);
				break;
			}
		}
		return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
	}

	void Hooker::create()
	{
		WNDCLASS wndClass = {};
		wndClass.hInstance = hModule;
		wndClass.lpfnWndProc = HookerWndProc;
		wndClass.lpszClassName = wsHookerName;
		RegisterClass(&wndClass);

		HWND hWnd = CreateWindow(wsHookerName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hModule, 0);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

		hMouseHook = SetWindowsHookEx(WH_MOUSE, &MouseHookProc, hModule, NULL);
	}

	typedef void(__cdecl* StartHookProc)(void* addin);

	static bool GetLibraryFile(std::wstring& path)
	{
		path.resize(MAX_PATH);
		while (true) {
			DWORD res = GetModuleFileName(hModule, path.data(), (DWORD)path.size());
			if (res && res < path.size()) return true;
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return false;
			path.resize(path.size() * 2);
		}
	}

	static HMODULE LoadHookLibrary()
	{
		std::wstring path;
		return GetLibraryFile(path) ? LoadLibrary(path.c_str()) : nullptr;
	}

	void Hook(void* addin)
	{
		if (auto h = LoadHookLibrary()) {
			auto proc = (StartHookProc)GetProcAddress(h, "HookEventMonitor");
			if (proc) proc(addin);
		}
	}
}

extern "C" {
	using namespace EventMonitor;

	__declspec(dllexport) void __cdecl HookEventMonitor(void* addin)
	{
		auto hWnd = FindWindow(wsHookerName, NULL);
		if (hWnd) PostMessage(hWnd, WM_DESTROY, 0, 0);
		if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
		if (addin) {
			Hooker* settings = new Hooker((AddInNative*)addin);
			CreateThread(0, NULL, HookerThreadProc, (LPVOID)settings, NULL, NULL);
		}
	}
}

#endif//_WINDOWS
