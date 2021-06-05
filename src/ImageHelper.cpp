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
	m_bitmap.reset(Gdiplus::Bitmap::FromHBITMAP(hBitmap, 0));
}

BaseHelper::ImageHelper::ImageHelper(VH variant)
{
	if (!GgiPlusToken::Init()) return;
	ComUniquePtr<IStream> stream(CreateMemoryStream((BYTE*)variant.data(), variant.size()));
	if (stream) m_bitmap.reset(Gdiplus::Bitmap::FromStream(stream.get()));
}

BaseHelper::ImageHelper::ImageHelper(const BITMAPINFO* gdiBitmapInfo, VOID* gdiBitmapData)
{
	if (!GgiPlusToken::Init()) return;
	m_bitmap.reset(Gdiplus::Bitmap::FromBITMAPINFO(gdiBitmapInfo, gdiBitmapData));
}

BaseHelper::ImageHelper::operator HBITMAP() const
{
	HBITMAP hbitmap = NULL;
	auto status = m_bitmap->GetHBITMAP(NULL, &hbitmap);
	if (status != Gdiplus::Ok) return NULL;
	return hbitmap;
}

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0) return -1;  // Failure

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL) return -1;  // Failure

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j) {
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

template<class T>
static bool SaveBitmap(Gdiplus::Bitmap* bitmap, T& out)
{
	if (bitmap == nullptr) return false;
	if (!GgiPlusToken::Init()) return false;

	CLSID clsid;
	GetEncoderClsid(L"image/png", &clsid);

	ComUniquePtr<IStream> stream(CreateMemoryStream(NULL, 0));
	if (stream) {
		auto status = bitmap->Save(stream.get(), &clsid, 0);
		if (status == Gdiplus::Ok) {
			const LARGE_INTEGER lOfs{ 0 };
			ULARGE_INTEGER lSize;
			if (SUCCEEDED(stream->Seek(lOfs, STREAM_SEEK_END, &lSize))) {
				if (SUCCEEDED(stream->Seek(lOfs, STREAM_SEEK_SET, 0))) {
					out.resize((unsigned long)lSize.QuadPart);
					if (SUCCEEDED(stream->Read(out.data(), (ULONG)lSize.QuadPart, nullptr))) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool BaseHelper::ImageHelper::Save(std::vector<BYTE>& vec)
{
	return SaveBitmap(m_bitmap.get(), vec);
}

bool BaseHelper::ImageHelper::Save(VH variant)
{
	return SaveBitmap(m_bitmap.get(), variant);
}

bool BaseHelper::ImageHelper::Scale(VH source, VH target, double factor)
{
	ImageHelper src(source);
	double w = factor * (double)src.m_bitmap->GetWidth();
	double h = factor * (double)src.m_bitmap->GetHeight();
	ImageHelper result((int)w, (int)h);
	Gdiplus::Graphics g(result.m_bitmap.get());
	g.ScaleTransform((Gdiplus::REAL)factor, (Gdiplus::REAL)factor);
	g.DrawImage(src.m_bitmap.get(), 0, 0);
	return result.Save(target);
}

bool BaseHelper::ImageHelper::Crop(VH source, VH target, int64_t x, int64_t y, int64_t w, int64_t h)
{
	ImageHelper src(source);
	ImageHelper result((int)w, (int)h);
	Gdiplus::Graphics g(result.m_bitmap.get());
	g.Clear(Gdiplus::Color::White);
	g.DrawImage(src.m_bitmap.get(), (float)-x, (float)-y);
	return result.Save(target);
}

#endif //_WINDOWS
