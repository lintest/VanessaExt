#pragma once

#ifdef _WINDOWS

#include "stdafx.h"

namespace EventMonitor {
	class Hooker;
	Hooker* getHooker(HWND hWnd);
	Hooker* getHooker(LPVOID lpParam);
	void Hook(void* addin);
	void Unhook();
}

#endif//_WINDOWS