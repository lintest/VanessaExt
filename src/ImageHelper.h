#ifndef __IMAGEHELPER_H__
#define __IMAGEHELPER_H__

#include "stdafx.h"

#include "IMemoryManager.h"

class ImageHelper
{
public:
	ImageHelper(IMemoryManager* iMemory) noexcept { m_iMemory = iMemory; }
	BOOL SaveBitmap(HBITMAP hBitmap, tVariant* pvarRetValue);
private:
	IMemoryManager* m_iMemory;
};

#endif //__IMAGEHELPER_H__