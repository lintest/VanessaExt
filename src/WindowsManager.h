#pragma once
#include "stdafx.h"

class WindowsManager {
public:
	static int64_t ActiveWindow();
	static int64_t ActivateProcess(int64_t pid);
	static int64_t GetTopProcessWindow(int64_t pid);
	static int64_t GetMainProcessWindow(int64_t pid);
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
	int64_t GetWindowState(int64_t window);
#ifdef _WINDOWS
	static int64_t FindWindow(const std::wstring& name, const std::wstring& title);
	static bool PostMessage(int64_t hWnd, int64_t Msg, int64_t wParam, int64_t lParam);
	static bool SetWindowState(HWND hWnd, int iMode, bool bActivate);
	static bool IsMaximized(HWND hWnd);
#else	
	static bool IsMaximized(int64_t window);
#endif//_WINDOWS
};
