#ifndef __WINDOWMNGR_H__
#define __WINDOWMNGR_H__

#include "stdafx.h"

class WindowManager {
public:
	static HWND ActiveWindow();
	static HWND CurrentWindow();
	static std::wstring GetWindowList(tVariant* paParams, const long lSizeArray);
	static std::wstring GetText(tVariant* paParams, const long lSizeArray);
	static long GetWindowState(tVariant* paParams, const long lSizeArray);
	static BOOL SetText(tVariant* paParams, const long lSizeArray);
	static BOOL SetWindowState(tVariant* paParams, const long lSizeArray);
	static BOOL SetWindowSize(tVariant* paParams, const long lSizeArray);
	static BOOL SetWindowPos(tVariant* paParams, const long lSizeArray);
	static BOOL EnableResizing(tVariant* paParams, const long lSizeArray);
	static BOOL Restore(tVariant* paParams, const long lSizeArray);
	static BOOL Maximize(tVariant* paParams, const long lSizeArray);
	static BOOL Minimize(tVariant* paParams, const long lSizeArray);
	static BOOL Activate(tVariant* paParams, const long lSizeArray);
private:
	static BOOL WindowManager::SetWindowState(HWND hWnd, int iMode, bool bActivate);
};

#endif //__WINDOWMNGR_H__
