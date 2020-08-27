#ifndef __BASEHELPER_H__
#define __BASEHELPER_H__

#include "AddInNative.h"

class BaseHelper
	: public AddInNative
{
protected:
	class ClipboardManager;
	class ScreenManager;
	class ImageHelper;
};

#endif // __BASEHELPER_H__