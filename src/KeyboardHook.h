#pragma once

#ifdef _WINDOWS
#include <windows.h>
#endif

#include "AddInNative.h"

namespace KeyboardHook {
	std::string Hook(AddInNative* addin, const std::u16string& msg = u"");
}
