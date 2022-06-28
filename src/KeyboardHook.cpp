#ifdef _WINDOWS

#include "stdafx.h"
#include "KeyboardHook.h"

static AddInNative* s_addin = nullptr;

static std::u16string s_message = u"HOTKEY";

static HHOOK hKeyboardHook = NULL;

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	WORD vkCode = LOWORD(wParam);
	WORD keyFlags = HIWORD(lParam);
	if (nCode == HC_ACTION) {
		if (s_addin && vkCode == 0x48
			&& (keyFlags & KF_ALTDOWN)
			&& !(keyFlags & KF_REPEAT)
			&& !(keyFlags & KF_UP))
		{
			s_addin->ExternalEvent(s_message.c_str(), u"ALT+H");
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

typedef void(__cdecl* SetKeyboardHookProc)(void* addin);

std::string KeyboardHook::Hook(AddInNative* addin, const std::u16string& msg)
{
	auto hLibrary = LoadHookLibrary();
	if (hLibrary) {
		auto proc = (SetKeyboardHookProc)GetProcAddress(hLibrary, "SetKeyboardHook");
		if (addin) s_message = msg.empty() ? u"HOTKEY" : msg;
		if (proc) { proc(addin); return {}; }
	}
	return "Keyboard hook error";
}

extern "C" {
	__declspec(dllexport) void __cdecl SetKeyboardHook(void* addin)
	{
		s_addin = (AddInNative*)addin;

		if (hKeyboardHook)
			UnhookWindowsHookEx(hKeyboardHook);

		hKeyboardHook = NULL;

		if (addin)
			hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, &KeyboardHookProc, hModule, NULL);
	}
}

#endif //_WINDOWS