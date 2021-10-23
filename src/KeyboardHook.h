#pragma once

#ifdef _WINDOWS
#include <windows.h>
#include "AddInNative.h"

class KeyboardHook {
private:
	struct HotKey {
		bool shift;
		bool ctrl;
		bool alt;
		std::wstring msg;
		std::wstring data;
	};
	std::multimap<DWORD, HotKey> map;
	AddInNative& addin;
public:
	KeyboardHook(AddInNative& addin, const std::string& json);
	void Notify(WPARAM wParam, LPARAM lParam);
public:
	static std::string Hook(AddInNative& addin, const std::string &json);
	static std::string Unhook();
};

#endif //_WINDOWS
