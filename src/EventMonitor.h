#pragma once

#ifdef _WINDOWS

#include "stdafx.h"

namespace EventMonitor {
	class Hooker;
	bool getStatus();
	Hooker* getHooker(HWND hWnd);
	Hooker* getHooker(LPVOID lpParam);
	void Hook(void* addin);
}

#endif//_WINDOWS