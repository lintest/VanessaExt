#ifndef SOUNDEFFECT_H
#define SOUNDEFFECT_H

#ifdef _WINDOWS
#include "windows.h"

class SoundEffect {
public:
	static BOOL PlaySound(const std::wstring& filename, bool async);
	static void PlayMedia(const std::wstring &uuid, const std::wstring & filename);
	static std::wstring MediaCommand(const std::wstring& command);
};

#endif //_WINDOWS

#endif //SOUNDEFFECT_H