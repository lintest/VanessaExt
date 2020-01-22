#ifndef __IMAGEHELPER_H__
#define __IMAGEHELPER_H__

#include "stdafx.h"
#include "IMemoryManager.h"

#ifdef _WINDOWS

#include <gdiplus.h>

class ImageHelper
{
public:
	ImageHelper(HBITMAP hBitmap);
	ImageHelper(tVariant* pvarValue);
	ImageHelper(const BITMAPINFO* gdiBitmapInfo, VOID* gdiBitmapData);
	virtual ~ImageHelper() { if (m_bitmap) delete m_bitmap; }
	BOOL Save(AddInNative* addin, tVariant* pvarRetValue);
	BOOL Save(std::vector<BYTE> &buffer);
	operator bool() const { return m_bitmap; }
	operator HBITMAP() const;
private:
	Gdiplus::Bitmap* m_bitmap = 0;
};

#endif //_WINDOWS

#endif //__IMAGEHELPER_H__
