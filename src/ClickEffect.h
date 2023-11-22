#pragma once

#ifdef _WINDOWS
#include <windows.h>

namespace ClickEffect
{
	class Hooker;
	class Painter;
	Hooker* getHooker(HWND hWnd);
	Hooker* getHooker(LPVOID lpParam);
	Painter* getPainter(HWND hWnd);
	Painter* getPainter(LPVOID lpParam);
	void Hook(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans, int64_t echo);
	void Show(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans, int64_t echo);
	void Unhook();
	void Show();
}

#endif //_WINDOWS
