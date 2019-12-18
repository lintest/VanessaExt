#ifndef __WINCTRL_H__
#define __WINCTRL_H__

#include <types.h>
#include <string>
#include "IMemoryManager.h"

class WindowsControl {
public:
	static std::wstring GetWindowList();
	static std::wstring GetText(tVariant* paParams, const long lSizeArray);
	static BOOL SetText(tVariant* paParams, const long lSizeArray);
	static BOOL SetWindowSize(tVariant* paParams, const long lSizeArray);
	static BOOL SetWindowPos(tVariant* paParams, const long lSizeArray);
	static BOOL EnableResizing(tVariant* paParams, const long lSizeArray);
	static BOOL Restore(tVariant* paParams, const long lSizeArray);
	static BOOL Maximize(tVariant* paParams, const long lSizeArray);
	static BOOL Minimize(tVariant* paParams, const long lSizeArray);
	static BOOL Activate(tVariant* paParams, const long lSizeArray);
	static HWND ActiveWindow();
	static HWND CurrentWindow();
public:
	WindowsControl(IMemoryManager* iMemory) { m_iMemory = iMemory; }
	BOOL CaptureScreen(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
	BOOL CaptureWindow(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
	BOOL CaptureWindow(tVariant* pvarRetValue, HWND hWnd);
private:
	BOOL SaveBitmap(HBITMAP hBitmap, tVariant* pvarRetValue);
	IMemoryManager* m_iMemory;
};

#endif //__WINCTRL_H__
