#ifndef __WINCTRL_H__
#define __WINCTRL_H__

#include <types.h>
#include <string>
#include "IMemoryManager.h"

#ifdef __linux__

class WindowsControl {
public:
	static std::wstring GetWindowList(tVariant* paParams, const long lSizeArray);
};

#else//__linux__

class WindowsControl {
public:
	WindowsControl(IMemoryManager* iMemory) { m_iMemory = iMemory; }
	static HWND ActiveWindow();
	static HWND CurrentWindow();
	static std::wstring GetScreenInfo();
	static std::wstring GetDisplayInfo(tVariant* paParams, const long lSizeArray);
	static std::wstring GetDisplayList(tVariant* paParams, const long lSizeArray);
	static std::wstring GetWindowList(tVariant* paParams, const long lSizeArray);
	static std::wstring GetChildWindows(tVariant* paParams, const long lSizeArray);
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
	BOOL CaptureScreen(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
	BOOL CaptureWindow(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
private:
#ifndef __linux__	
	BOOL SaveBitmap(HBITMAP hBitmap, tVariant* pvarRetValue);
#endif//__linux__
	static BOOL SetWindowState(HWND hWnd, int iMode, bool bActivate);
	BOOL CaptureWindow(tVariant* pvarRetValue, HWND hWnd);
	IMemoryManager* m_iMemory;
};

#endif//__linux__

#endif //__WINCTRL_H__
