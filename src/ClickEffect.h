#ifndef CLICKEFFECT_H
#define CLICKEFFECT_H

#ifdef _WINDOWS
#include "windows.h"

class ClickEffect {
public:
	class Hooker;
	class Painter;
	static Hooker* hooker(HWND hWnd);
	static Hooker* hooker(LPVOID lpParam);
	static Painter* painter(HWND hWnd);
	static Painter* painter(LPVOID lpParam);
	static void Hook(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans);
	static void Show(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans);
	static void Unhook();
	static void Show();
};

#endif //_WINDOWS

#endif //CLICKEFFECT_H