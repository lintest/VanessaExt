#ifdef _WINDOWS

#include "stdafx.h"
#include "KeyboardHook.h"

void KeyboardHook::Notify(WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT key = *((KBDLLHOOKSTRUCT*)lParam);
	if (key.vkCode == VK_PAUSE) {
		addin.ExternalEvent(u"KEYBOARD_HOOK", u"PAUSE");
		auto SHIFT_key = GetAsyncKeyState(VK_SHIFT);
		auto CTRL_key = GetAsyncKeyState(VK_CONTROL);
		auto ALT_key = GetAsyncKeyState(VK_MENU);
	}
}

static std::unique_ptr<KeyboardHook> hooker;

const UINT WM_KEYBOARD_HOOK = WM_USER + 2;

const LPCWSTR wsKeyboardHookName = L"VanessaKeyboardHooker";

static HANDLE hKeyboardHookThread = NULL;

static HHOOK hKeyboardHook = NULL;

static HWND hKeyboardWnd = NULL;

LRESULT CALLBACK KeyboardHookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYBOARD_HOOK:
		if (hooker) hooker->Notify(wParam, lParam);
		return 1;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		switch (wParam) {
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			::SendMessage(hKeyboardWnd, WM_KEYBOARD_HOOK, wParam, lParam);
			break;
		}
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

static DWORD WINAPI KeyboardHookThreadProc(LPVOID lpParam)
{
	WNDCLASS wndClass = {};
	wndClass.hInstance = hModule;
	wndClass.lpfnWndProc = KeyboardHookWndProc;
	wndClass.lpszClassName = wsKeyboardHookName;
	RegisterClass(&wndClass);

	hKeyboardWnd = CreateWindow(wsKeyboardHookName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hModule, 0);
	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHookProc, hModule, NULL);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (hKeyboardHook)
		UnhookWindowsHookEx(hKeyboardHook);

	hKeyboardWnd = NULL;
	hKeyboardHook = NULL;

	return 0;
}

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

typedef void(__cdecl* StartHookProc)(AddInNative& addin, const std::string& json);

typedef void(__cdecl* StopHookProc)();

void KeyboardHook::Hook(AddInNative& addin, const std::string& json)
{
	if (auto h = LoadHookLibrary()) {
		auto proc = (StartHookProc)GetProcAddress(h, "StartKeyboardHook");
		if (proc) proc(addin, json);
	}
}

void KeyboardHook::Unhook()
{
	if (auto h = LoadHookLibrary()) {
		auto proc = (StopHookProc)GetProcAddress(h, "StopKeyboardHook");
		if (proc) proc();
	}
}

extern "C" {
	__declspec(dllexport) void __cdecl StopKeyboardHook()
	{
		if (hKeyboardWnd)
			PostMessage(hKeyboardWnd, WM_DESTROY, 0, 0);

		if (hKeyboardHookThread)
			WaitForSingleObject(hKeyboardHookThread, INFINITE);

		hKeyboardHookThread = NULL;
	}
	__declspec(dllexport) void __cdecl StartKeyboardHook(AddInNative& addin, const std::string& json)
	{
		StopKeyboardHook();
		hooker.reset(new KeyboardHook(addin, json));
		hKeyboardHookThread = CreateThread(0, NULL, KeyboardHookThreadProc, NULL, NULL, NULL);
	}
}

#endif //_WINDOWS