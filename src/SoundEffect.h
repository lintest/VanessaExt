#pragma once

#ifdef _WINDOWS
#include <windows.h>

class SoundEffect {
public:
	static std::wstring MediaCommand(const std::wstring& command);
	static BOOL PlaySound(const std::wstring& filename, bool async);
	static void PlayMedia(const std::wstring& uuid, const std::wstring& filename);
	static bool PlayingMedia(const std::wstring& uuid);
};

#endif //_WINDOWS
