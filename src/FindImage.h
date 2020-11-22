#ifndef __IMAGEFINDER_H__
#define __IMAGEFINDER_H__

#ifdef _WINDOWS

#include "stdafx.h"
#include "json.hpp"

class ImageFinder {
public:	
	static std::string find(const std::string &picture, const std::string &fragment, int match_method = 0);
};

#endif //_WINDOWS

#endif//__IMAGEFINDER_H__
