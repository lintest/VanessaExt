#include "ImageHelper.h"

#ifdef _WINDOWS

#pragma comment(lib, "gdiplus.lib")

class GgiPlusToken {
private:
	ULONG_PTR h = NULL;
public:
	static bool Init();
	GgiPlusToken() noexcept {}
	~GgiPlusToken() { if (h) Gdiplus::GdiplusShutdown(h); }
	ULONG_PTR* operator &() noexcept { return &h; }
	BOOL operator!() noexcept { return !h; }
};

static GgiPlusToken gdiplusToken;

bool GgiPlusToken::Init()
{
	Gdiplus::Status status = Gdiplus::Ok;
	if (!gdiplusToken) // initialization of gdi+
	{
		const Gdiplus::GdiplusStartupInput input;
		status = Gdiplus::GdiplusStartup(&gdiplusToken, &input, NULL);
		if (status != Gdiplus::Ok) return false;
	}
	return true;
}

typedef IStream* (STDAPICALLTYPE* SHCreateMemStreamType)(const BYTE* pInit, UINT cbInit);

static IStream* CreateMemoryStream(const BYTE* pInit, UINT cbInit)
{
	static SHCreateMemStreamType SHCreateMemStreamFunc = nullptr;
	if (!SHCreateMemStreamFunc) {
		HMODULE lib = LoadLibrary(L"shlwapi.dll");
		SHCreateMemStreamFunc = reinterpret_cast<SHCreateMemStreamType>(GetProcAddress(lib, "SHCreateMemStream"));
	}
	if (SHCreateMemStreamFunc) return SHCreateMemStreamFunc(pInit, cbInit);
	return nullptr;
}

ImageHelper::ImageHelper(HBITMAP hBitmap)
{
	if (!GgiPlusToken::Init()) return;
	m_bitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, 0);
}

ImageHelper::ImageHelper(tVariant* pvarValue)
{
	if (!GgiPlusToken::Init()) return;
	if (IStream* pStream = CreateMemoryStream((BYTE*)pvarValue->pstrVal, pvarValue->strLen)) {
		m_bitmap = Gdiplus::Bitmap::FromStream(pStream);
		pStream->Release(); // releasing stream
	}
}

ImageHelper::ImageHelper(const BITMAPINFO* gdiBitmapInfo, VOID* gdiBitmapData)
{
	if (!GgiPlusToken::Init()) return;
	m_bitmap = Gdiplus::Bitmap::FromBITMAPINFO(gdiBitmapInfo, gdiBitmapData);
}

ImageHelper::operator HBITMAP() const
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

BOOL ImageHelper::Save(std::vector<BYTE>& vec)
{
	if (!GgiPlusToken::Init()) return false;

	CLSID clsid;
	GetEncoderClsid(L"image/png", &clsid); // retrieving JPEG encoder CLSID

	BOOL Ret = FALSE;
	if (IStream* pStream = CreateMemoryStream(NULL, 0)) // creating stream
	{
		Gdiplus::Status status = m_bitmap->Save(pStream, &clsid, 0); // saving image to the stream
		if (status == Gdiplus::Ok) {
			const LARGE_INTEGER lOfs{0};
			ULARGE_INTEGER lSize;
			if (SUCCEEDED(pStream->Seek(lOfs, STREAM_SEEK_END, &lSize))) // retrieving size of stream data (seek to end)
			{
				vec.resize(lSize.QuadPart);
				if (SUCCEEDED(pStream->Seek(lOfs, STREAM_SEEK_SET, 0))) // seeking to beginning of the stream data
				{
					if (SUCCEEDED(pStream->Read(vec.data(), (ULONG)vec.size(), 0))) // reading stream to buffer
					{
						Ret = TRUE;
					}
				}
			}
		}
		pStream->Release(); // releasing stream
	}
	return Ret;
}

BOOL ImageHelper::Save(AddInNative* addin, tVariant* pvarRetValue)
{
	if (!GgiPlusToken::Init()) return false;

	CLSID clsid;
	GetEncoderClsid(L"image/png", &clsid); // retrieving JPEG encoder CLSID

	BOOL Ret = FALSE;
	if (IStream* pStream = CreateMemoryStream(NULL, 0)) // creating stream
	{
		Gdiplus::Status status = m_bitmap->Save(pStream, &clsid, 0); // saving image to the stream
		if (status == Gdiplus::Ok)
		{
			const LARGE_INTEGER lOfs{ 0 };
			ULARGE_INTEGER lSize;
			if (SUCCEEDED(pStream->Seek(lOfs, STREAM_SEEK_END, &lSize))) // retrieving size of stream data (seek to end)
			{
				if (SUCCEEDED(pStream->Seek(lOfs, STREAM_SEEK_SET, 0))) // seeking to beginning of the stream data
				{
					pvarRetValue->strLen = (ULONG)((DWORD_PTR)lSize.QuadPart);
					addin->AllocMemory((void**)&pvarRetValue->pstrVal, pvarRetValue->strLen);
					TV_VT(pvarRetValue) = VTYPE_BLOB;
					if (pvarRetValue->pstrVal)
					{
						if (SUCCEEDED(pStream->Read(pvarRetValue->pstrVal, pvarRetValue->strLen, 0))) // reading stream to buffer
						{
							Ret = TRUE;
						}
					}
				}
			}
		}
		pStream->Release(); // releasing stream
	}
	return Ret;
}

#endif //_WINDOWS
