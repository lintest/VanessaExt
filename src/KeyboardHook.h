#pragma once

#ifdef _WINDOWS
#include <windows.h>
#include "AddInNative.h"

class KeyboardHook {
private:
	AddInNative& addin;
public:
	KeyboardHook(AddInNative& addin, const std::string& json) : addin(addin) {}
	void Notify(WPARAM wParam, LPARAM lParam);
public:
	static void Hook(AddInNative& addin, const std::string &json);
	static void Unhook();
};

#endif //_WINDOWS
