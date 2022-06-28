#ifdef _WINDOWS

#include "stdafx.h"
#include "KeyboardHook.h"

static AddInNative* hooker = nullptr;

static HHOOK hKeyboardHook = NULL;

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	WORD vkCode = LOWORD(wParam);
	WORD keyFlags = HIWORD(lParam);
	if (nCode == HC_ACTION) {
		if (hooker && vkCode == 0x48
			&& (keyFlags & KF_ALTDOWN)
			&& !(keyFlags & KF_REPEAT)
			&& !(keyFlags & KF_UP))
		{
			hooker->ExternalEvent(u"HOTKEY", u"ALT+H");
		}
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
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

std::string KeyboardHook::Hook(AddInNative* addin)
{
	hooker = addin;
	auto hLibrary = LoadHookLibrary();
	if (hLibrary) {
		auto proc = (StartHookProc)GetProcAddress(hLibrary, 
			addin ? "StartKeyboardHook" : "StopKeyboardHook");
		if (proc) { proc(); return {}; }
	}
	return "Keyboard hook error";
}

extern "C" {
	__declspec(dllexport) void __cdecl StopKeyboardHook()
	{
		if (hKeyboardHook)
			UnhookWindowsHookEx(hKeyboardHook);

		hKeyboardHook = NULL;
	}
	__declspec(dllexport) void __cdecl StartKeyboardHook()
	{
		StopKeyboardHook();
		hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, &KeyboardHookProc, hModule, NULL);
	}
}

#endif //_WINDOWS