#ifndef __BASEHELPER_H__
#define __BASEHELPER_H__

#include "AddInNative.h"
#include "stdafx.h"

class BaseHelper
	: public AddInNative
{
protected:
	class ClipboardManager;
	class ScreenManager;
	class ImageHelper;
	class ImageFinder;
};

#endif // __BASEHELPER_H__