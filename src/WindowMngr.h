#ifndef __WINDOWMNGR_H__
#define __WINDOWMNGR_H__

#include "stdafx.h"

class WindowManager {
public:
	static HWND ActiveWindow();
	static HWND CurrentWindow();
	static std::string GetWindowList(int64_t pid);
	static std::string GetWindowInfo(int64_t window);
	static std::string GetWindowSize(int64_t window);
	static bool EnableResizing(int64_t window, bool enable);
	static bool SetWindowPos(int64_t window, int64_t x, int64_t y, int64_t w, int64_t h);
	static bool SetWindowSize(int64_t window, int64_t w, int64_t h);
	static bool SetWindowState(int64_t window, int64_t mode, bool activate);
	static bool Restore(int64_t window);
	static bool Maximize(int64_t window);
	static bool Minimize(int64_t window);
	static bool Activate(int64_t window);
private:
	static bool SetWindowState(HWND hWnd, int iMode, bool bActivate);
	int64_t GetWindowState(int64_t window);
	static bool IsMaximized(HWND hWnd);
};

#endif //__WINDOWMNGR_H__
