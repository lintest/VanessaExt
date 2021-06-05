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
	static bool Scale(VH source, VH target, double factor);
	static bool Crop(VH source, VH target, int64_t x, int64_t y, int64_t w, int64_t h);
	bool Save(VH variant);
	bool Save(std::vector<BYTE>& buffer);
	operator bool() const { return (bool)m_bitmap; }
	operator HBITMAP() const;
private:
	std::unique_ptr<Gdiplus::Bitmap> m_bitmap;
};
#endif //_WINDOWS
