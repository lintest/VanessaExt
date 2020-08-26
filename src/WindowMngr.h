#ifndef __WINDOWMNGR_H__
#define __WINDOWMNGR_H__

#include "stdafx.h"

class WindowManager {
public:
	static HWND ActiveWindow();
	static HWND CurrentWindow();
	static std::string GetWindowList(int32_t pid);
	static std::string GetWindowInfo(int32_t window);
	static std::string GetWindowSize(int32_t window);
	static bool EnableResizing(int32_t window, bool enable);
	static bool SetWindowPos(int32_t window, int32_t x, int32_t y, int32_t w, int32_t h);
	static bool SetWindowSize(int32_t window, int32_t w, int32_t h);
	static bool SetWindowState(int32_t window, int32_t mode, bool activate);
	static bool Restore(int32_t window);
	static bool Maximize(int32_t window);
	static bool Minimize(int32_t window);
	static bool Activate(int32_t window);
private:
	static bool SetWindowState(HWND hWnd, int iMode, bool bActivate);
	int32_t GetWindowState(int32_t window);
	static bool IsMaximized(HWND hWnd);
};

#endif //__WINDOWMNGR_H__
