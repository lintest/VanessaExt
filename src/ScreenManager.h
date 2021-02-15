#pragma once
#include "stdafx.h"
#include "AddInNative.h"
#include "BaseHelper.h"

#ifndef _WINDOWS
#include <X11/Xlib.h>
#endif

class BaseHelper::ScreenManager
{
private:
#ifdef _WINDOWS
	static BOOL Capture(VH variant, HWND window);
#else
	static BOOL Save(XImage* image, VH &variant, int width, int height);
	static BOOL Capture(VH variant, Window win);
#endif//_WINDOWS
private:
	class Hotkey;
public:
	ScreenManager() { }
	static std::string GetScreenInfo();
	static std::string GetScreenList();
	static std::string GetDisplayInfo(int64_t window);
	static std::string GetDisplayList(int64_t window);
	static BOOL CaptureProcess(VH variant, int64_t pid);
	static BOOL CaptureScreen(VH variant, int64_t mode);
	static BOOL CaptureWindow(VH variant, int64_t window);
	static BOOL CaptureRegion(VH variant, int64_t x, int64_t y, int64_t w, int64_t h);
	static std::string GetCursorPos();
	static BOOL SetCursorPos(int64_t x, int64_t y);
	static BOOL EmulateClick(int64_t button, VH keys);
	static BOOL EmulateDblClick(int64_t delay = 100);
	static BOOL EmulateHotkey(VH keys, int64_t flags);
	static BOOL EmulateMouse(int64_t X, int64_t Y, int64_t C, int64_t P, bool button);
	static BOOL EmulateWheel(int64_t sign, VH variant);
	static BOOL EmulateText(const std::wstring& text, int64_t pause);
};
