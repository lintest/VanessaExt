#include "ImageHelper.h"

#ifdef _WINDOWS

#pragma comment(lib, "gdiplus.lib")

GgiPlusToken GgiPlusToken::instance;

bool GgiPlusToken::Init()
{
	Gdiplus::Status status = Gdiplus::Ok;
	if (!instance) {
		const Gdiplus::GdiplusStartupInput input;
		status = Gdiplus::GdiplusStartup(&instance, &input, NULL);
		if (status != Gdiplus::Ok) return false;
	}
	return true;
}

typedef IStream* (STDAPICALLTYPE* SHCreateMemStreamType)(const BYTE* pInit, UINT cbInit);

static IStream* CreateMemoryStream(const BYTE* pInit, UINT cbInit)
{
	static SHCreateMemStreamType SHCreateMemStreamFunc = nullptr;
	if (!SHCreateMemStreamFunc) {
		if (auto lib = LoadLibrary(L"shlwapi.dll")) {
			SHCreateMemStreamFunc = reinterpret_cast<SHCreateMemStreamType>(GetProcAddress(lib, "SHCreateMemStream"));
		}
	}
	if (SHCreateMemStreamFunc) return SHCreateMemStreamFunc(pInit, cbInit);
	return nullptr;
}

BaseHelper::ImageHelper::ImageHelper(HBITMAP hBitmap)
{
	if (!GgiPlusToken::Init()) return;
	m_bitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, 0);
}

BaseHelper::ImageHelper::ImageHelper(VH variant)
{
	if (!GgiPlusToken::Init()) return;
	ComUniquePtr<IStream> stream(CreateMemoryStream((BYTE*)variant.data(), variant.size()));
	if (stream) m_bitmap = Gdiplus::Bitmap::FromStream(stream.get());
}

BaseHelper::ImageHelper::ImageHelper(const BITMAPINFO* gdiBitmapInfo, VOID* gdiBitmapData)
{
	if (!GgiPlusToken::Init()) return;
	m_bitmap = Gdiplus::Bitmap::FromBITMAPINFO(gdiBitmapInfo, gdiBitmapData);
}

BaseHelper::ImageHelper::operator HBITMAP() const
{
	HBITMAP hbitmap = NULL;
	auto status = m_bitmap->GetHBITMAP(NULL, &hbitmap);
	if (status != Gdiplus::Ok) return NULL;
	return hbitmap;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0) return -1;  // Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL) return -1;  // Failure

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

BOOL BaseHelper::ImageHelper::Save(std::vector<BYTE>& vec)
{
	if (!GgiPlusToken::Init()) return false;

	CLSID clsid;
	GetEncoderClsid(L"image/png", &clsid);

	BOOL Ret = FALSE;
	ComUniquePtr<IStream> stream(CreateMemoryStream(NULL, 0));
	if (stream) {
		auto status = m_bitmap->Save(stream.get(), &clsid, 0);
		if (status == Gdiplus::Ok) {
			const LARGE_INTEGER lOfs{0};
			ULARGE_INTEGER lSize;
			if (SUCCEEDED(stream->Seek(lOfs, STREAM_SEEK_END, &lSize))) {
				vec.resize((size_t)lSize.QuadPart);
				if (SUCCEEDED(stream->Seek(lOfs, STREAM_SEEK_SET, 0))) {
					if (SUCCEEDED(stream->Read(vec.data(), (ULONG)vec.size(), 0))) {
						Ret = TRUE;
					}
				}
			}
		}
	}
	return Ret;
}

BOOL BaseHelper::ImageHelper::Save(VH variant)
{
	if (!GgiPlusToken::Init()) return false;

	CLSID clsid;
	GetEncoderClsid(L"image/png", &clsid);

	BOOL Ret = FALSE;
	ComUniquePtr<IStream> stream(CreateMemoryStream(NULL, 0));
	if (stream) {
		auto status = m_bitmap->Save(stream.get(), &clsid, 0);
		if (status == Gdiplus::Ok) {
			const LARGE_INTEGER lOfs{ 0 };
			ULARGE_INTEGER lSize;
			if (SUCCEEDED(stream->Seek(lOfs, STREAM_SEEK_END, &lSize))) {
				if (SUCCEEDED(stream->Seek(lOfs, STREAM_SEEK_SET, 0))) {
					variant.AllocMemory((unsigned long)lSize.QuadPart);
					if (SUCCEEDED(stream->Read(variant.data(), (ULONG)lSize.QuadPart, nullptr))) {
						Ret = TRUE;
					}
				}
			}
		}
	}
	return Ret;
}

BOOL BaseHelper::ImageHelper::Scale(VH source, VH target, double factor)
{
	ImageHelper src(source);
	double w = factor * (double)src.m_bitmap->GetWidth();
	double h = factor * (double)src.m_bitmap->GetHeight();
	ImageHelper result((int)w, (int)h);
	Gdiplus::Graphics g(result.m_bitmap);
	g.ScaleTransform((Gdiplus::REAL)factor, (Gdiplus::REAL)factor);
	g.DrawImage(src.m_bitmap, 0, 0);
	result.Save(target);
	return false;
}

BOOL BaseHelper::ImageHelper::Crop(VH source, VH target, int64_t x, int64_t y, int64_t w, int64_t h)
{
	ImageHelper src(source);
	ImageHelper result((int)w, (int)h);
	Gdiplus::Graphics g(result.m_bitmap);
	g.Clear(Gdiplus::Color::White);
	g.DrawImage(src.m_bitmap, (float)-x, (float)-y);
	result.Save(target);
	return false;
}

#endif //_WINDOWS
