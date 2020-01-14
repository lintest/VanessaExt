#ifndef __IMAGEHELPER_H__
#define __IMAGEHELPER_H__

#include "stdafx.h"
#include <gdiplus.h>

#include "IMemoryManager.h"

class ImageHelper
{
public:
	ImageHelper(HBITMAP hBitmap) { m_bitmap = &Gdiplus::Bitmap(hBitmap, 0); }
	ImageHelper(const BITMAPINFO* gdiBitmapInfo, VOID* gdiBitmapData);
	virtual ~ImageHelper() { if (m_bitmap) delete m_bitmap; }
	BOOL Save(IMemoryManager* iMemory, tVariant* pvarRetValue);
	operator bool() const { return m_bitmap; }
private:
	Gdiplus::Bitmap* m_bitmap = 0;
};

#endif //__IMAGEHELPER_H__