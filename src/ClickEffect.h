#ifndef CLICKEFFECT_H
#define CLICKEFFECT_H

#ifdef _WINDOWS
#include "windows.h"

class ClickEffect {
private:
	HMODULE hModule;
public:
	ClickEffect(HMODULE hModule) : hModule(hModule) {}
	friend DWORD WINAPI ThreadProc(LPVOID lpParam);
	HANDLE Show(int64_t x, int64_t y);
};

#endif //_WINDOWS

#endif //CLICKEFFECT_H