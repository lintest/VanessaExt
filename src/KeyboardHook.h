#pragma once

#ifdef _WINDOWS
#include <windows.h>
#include "AddInNative.h"

namespace KeyboardHook {
	std::string Hook(AddInNative* addin, const std::u16string& msg = u"");
}

#endif //_WINDOWS
