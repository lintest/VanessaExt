#pragma once

#ifdef USE_OPENCV

#include "stdafx.h"
#include "BaseHelper.h"

class BaseHelper::ImageFinder {
public:
	static std::string find(VH picture, VH fragment, int method = 0);
};

#endif//USE_OPENCV
