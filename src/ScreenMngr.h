#ifndef __SCREENMNGR_H__
#define __SCREENMNGR_H__

#include "stdafx.h"

#include "AddInNative.h"

class ScreenManager
	: public AddInNative 
{
public:
	ScreenManager() { }
	static std::string GetScreenInfo();
	static std::string GetScreenList();
	static std::string GetDisplayInfo(int32_t window);
	static std::string GetDisplayList(int32_t window);
	BOOL CaptureProcess(VH variant, int32_t pid);
	BOOL CaptureScreen(VH variant, int32_t mode);
	BOOL CaptureWindow(VH variant, HWND window);
public:
	static std::wstring GetCursorPos();
	static BOOL SetCursorPos(int32_t x, int32_t y);
	static BOOL EmulateClick(int32_t button, VH keys);
	static BOOL EmulateDblClick();
	static BOOL EmulateHotkey(VH keys, int32_t flags);
	static BOOL EmulateMouse(int32_t X, int32_t Y, int32_t C, int32_t P);
	static BOOL EmulateWheel(int32_t sign, VH variant);
	static BOOL EmulateText(const std::wstring& text, int32_t pause);
};

#endif //__SCREENMNGR_H__
