#pragma once
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
