#pragma once
#include "stdafx.h"
#include "BaseHelper.h"

#ifdef _WINDOWS

#include <gdiplus.h>

class GgiPlusToken {
private:
	static GgiPlusToken instance;
	ULONG_PTR h = NULL;
public:
	static bool Init();
	GgiPlusToken() noexcept {}
	~GgiPlusToken() { if (h) Gdiplus::GdiplusShutdown(h); }
	ULONG_PTR* operator &() noexcept { return &h; }
	BOOL operator!() noexcept { return !h; }
};

class BaseHelper::ImageHelper {
public:
	ImageHelper(VH variant);
	ImageHelper(HBITMAP hBitmap);
	ImageHelper(int w, int h): m_bitmap(new Gdiplus::Bitmap(w, h)) {}
	ImageHelper(const BITMAPINFO* gdiBitmapInfo, VOID* gdiBitmapData);
	virtual ~ImageHelper() { if (m_bitmap) delete m_bitmap; }
	static BOOL Scale(VH source, VH target, double factor);
	static BOOL Crop(VH source, VH target, int64_t x, int64_t y, int64_t w, int64_t h);
	BOOL Save(VH variant);
	BOOL Save(std::vector<BYTE>& buffer);
	operator bool() const { return m_bitmap; }
	operator HBITMAP() const;
private:
	Gdiplus::Bitmap* m_bitmap = 0;
};
#endif //_WINDOWS
