#ifndef __WINDOWMNGR_H__
#define __WINDOWMNGR_H__

#include "stdafx.h"

class WindowManager {
public:
	static HWND ActiveWindow();
	static int64_t ActivateProcess(tVariant* paParams, const long lSizeArray);
	static int64_t GetProcessWindow(tVariant* paParams, const long lSizeArray);
	static std::wstring GetWindowList(tVariant* paParams, const long lSizeArray);
	static std::wstring GetWindowInfo(tVariant* paParams, const long lSizeArray);
	static std::wstring GetWindowSize(tVariant* paParams, const long lSizeArray);
	static bool SetWindowState(tVariant* paParams, const long lSizeArray);
	static bool SetWindowSize(tVariant* paParams, const long lSizeArray);
	static bool SetWindowPos(tVariant* paParams, const long lSizeArray);
	static bool EnableResizing(tVariant* paParams, const long lSizeArray);
	static bool Restore(tVariant* paParams, const long lSizeArray);
	static bool Maximize(tVariant* paParams, const long lSizeArray);
	static bool Minimize(tVariant* paParams, const long lSizeArray);
	static bool Activate(tVariant* paParams, const long lSizeArray);
private:
	static bool SetWindowState(HWND hWnd, int iMode, bool bActivate);
	int32_t GetWindowState(tVariant* paParams, const long lSizeArray);
	static bool IsMaximized(HWND hWnd);
};

#endif //__WINDOWMNGR_H__
