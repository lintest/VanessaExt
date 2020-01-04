#ifndef __SCREENMNGR_H__
#define __SCREENMNGR_H__

#ifndef _WINDOWS

#include "stdafx.h"

#include "IMemoryManager.h"

class ScreenManager {
public:
	ScreenManager(IMemoryManager* iMemory) { m_iMemory = iMemory; }
	static std::wstring GetScreenInfo();
	static std::wstring GetScreenList();
	static std::wstring GetDisplayInfo(tVariant* paParams, const long lSizeArray);
	static std::wstring GetDisplayList(tVariant* paParams, const long lSizeArray);
	BOOL CaptureScreen(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
	BOOL CaptureWindow(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
private:
#ifndef __linux__	
	BOOL SaveBitmap(HBITMAP hBitmap, tVariant* pvarRetValue);
#endif//__linux__
	BOOL CaptureWindow(tVariant* pvarRetValue, HWND hWnd);
	IMemoryManager* m_iMemory;
};

#endif //_WINDOWS

#endif //__SCREENMNGR_H__
