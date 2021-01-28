#ifndef DESKTOPMANAGER_H
#define DESKTOPMANAGER_H

#ifdef _WINDOWS

#include <windows.h>

class DesktopManager
{
public:
	static int64_t GetDesktopCount();
	static int64_t CreateDesktopNumber();
	static int64_t GetCurrentDesktopNumber();
	static bool GoToDesktopNumber(int64_t number);
	static bool RemoveDesktopByNumber(int64_t number, int64_t fallback);
	static int64_t GetWindowDesktopNumber(int64_t window);
	static bool MoveWindowToDesktopNumber(int64_t window, int64_t number);
};

#endif//_WINDOWS

#endif//DESKTOPMANAGER_H
