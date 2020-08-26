#ifndef __IMAGEHELPER_H__
#define __IMAGEHELPER_H__

#include "stdafx.h"
#include "AddInNative.h"

#ifdef _WINDOWS

#include <gdiplus.h>

class ImageHelper
	: public AddInNative
{
public:
	ImageHelper(VH variant);
	ImageHelper(HBITMAP hBitmap);
	ImageHelper(const BITMAPINFO* gdiBitmapInfo, VOID* gdiBitmapData);
	virtual ~ImageHelper() { if (m_bitmap) delete m_bitmap; }
	BOOL Save(VH variant);
	BOOL Save(std::vector<BYTE> &buffer);
	operator bool() const { return m_bitmap; }
	operator HBITMAP() const;
private:
	Gdiplus::Bitmap* m_bitmap = 0;
};

#endif //_WINDOWS

#endif //__IMAGEHELPER_H__
