#ifdef _WINDOWS

#include "stdafx.h"
#include "KeyboardHook.h"

KeyboardHook::KeyboardHook(AddInNative& addin, const std::string& text)
	: addin(addin) 
{
	auto json = JSON::parse(text);
	for (auto& j : json) {
		HotKey hotkey;
		DWORD key = (int)j["key"];
		hotkey.shift = j["shift"];
		hotkey.ctrl = j["ctrl"];
		hotkey.alt = j["alt"];
		hotkey.msg = MB2WC(j["msg"]);
		hotkey.data = MB2WC(j["data"]);
		map.insert(std::make_pair(key, hotkey));
	}
}

void KeyboardHook::Notify(WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT key = *((KBDLLHOOKSTRUCT*)lParam);
	auto range = map.equal_range(key.vkCode);
	if (range.first != range.second) {
		auto shift = GetAsyncKeyState(VK_SHIFT) < 0;
		auto ctrl = GetAsyncKeyState(VK_CONTROL) < 0;
		auto alt = GetAsyncKeyState(VK_MENU) < 0;
		for (auto& it = range.first; it != range.second; ++it) {
			auto key = it->second;
			if (shift == key.shift && ctrl == key.ctrl && alt == key.alt) {
				addin.ExternalEvent((char16_t*)key.msg.c_str(), (char16_t*)key.data.c_str());
			}
		}
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

typedef void(__cdecl* StartHookProc)();

typedef void(__cdecl* StopHookProc)();

std::string KeyboardHook::Hook(AddInNative& addin, const std::string& json)
{
	try {
		hooker.reset(new KeyboardHook(addin, json));
	}
	catch (nlohmann::json::parse_error&) {
		return "JSON parse error";
	}
	if (auto h = LoadHookLibrary()) {
		auto proc = (StartHookProc)GetProcAddress(h, "StartKeyboardHook");
		if (proc) { proc(); return {}; }
	}
	return "Keyboard hook error";
}

std::string KeyboardHook::Unhook()
{
	if (auto h = LoadHookLibrary()) {
		auto proc = (StopHookProc)GetProcAddress(h, "StopKeyboardHook");
		if (proc) { proc(); return {}; }
	}
	return "Keyboard hook error";
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
	__declspec(dllexport) void __cdecl StartKeyboardHook()
	{
		StopKeyboardHook();
		hKeyboardHookThread = CreateThread(0, NULL, KeyboardHookThreadProc, NULL, NULL, NULL);
	}
}

#endif //_WINDOWS