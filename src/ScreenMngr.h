#ifndef __SCREENMNGR_H__
#define __SCREENMNGR_H__

#include "stdafx.h"

#include "IMemoryManager.h"

class ScreenManager {
public:
	ScreenManager(AddInNative* addin) : m_addin(addin) { }
	static std::wstring GetScreenInfo();
	static std::wstring GetScreenList();
	static std::wstring GetDisplayInfo(tVariant* paParams, const long lSizeArray);
	static std::wstring GetDisplayList(tVariant* paParams, const long lSizeArray);
	BOOL CaptureScreen(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
	BOOL CaptureWindow(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
	BOOL CaptureProcess(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
private:
	BOOL CaptureWindow(tVariant* pvarRetValue, HWND hWnd);
	AddInNative* m_addin;
};

#endif //__SCREENMNGR_H__
