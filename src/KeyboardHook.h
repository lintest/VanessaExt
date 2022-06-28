#pragma once

#ifdef _WINDOWS
#include <windows.h>
#include "AddInNative.h"

namespace KeyboardHook {
	std::string Hook(AddInNative* addin);
}

#endif //_WINDOWS
