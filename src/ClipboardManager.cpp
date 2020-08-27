#include "ClipboardManager.h"
#include "json.hpp"

#ifdef _WINDOWS

#include "ImageHelper.h"

BaseHelper::ClipboardManager::ClipboardManager()
{
	m_isOpened = OpenClipboard(nullptr);
}

BaseHelper::ClipboardManager::~ClipboardManager()
{
	if (m_isOpened) CloseClipboard();
}

const std::map<int, std::string> BaseHelper::ClipboardManager::sm_formats{
	{ CF_TEXT            , "TEXT"            },
	{ CF_BITMAP          , "BITMAP"        	 },
	{ CF_METAFILEPICT    , "METAFILEPICT"  	 },
	{ CF_SYLK            , "SYLK"          	 },
	{ CF_DIF             , "DIF"           	 },
	{ CF_TIFF            , "TIFF"          	 },
	{ CF_OEMTEXT         , "OEMTEXT"       	 },
	{ CF_DIB             , "DIB"           	 },
	{ CF_PALETTE         , "PALETTE"       	 },
	{ CF_PENDATA         , "PENDATA"       	 },
	{ CF_RIFF            , "RIFF"          	 },
	{ CF_WAVE            , "WAVE"          	 },
	{ CF_UNICODETEXT     , "UNICODETEXT"   	 },
	{ CF_ENHMETAFILE     , "ENHMETAFILE"   	 },
	{ CF_HDROP           , "HDROP"         	 },
	{ CF_LOCALE          , "LOCALE"        	 },
	{ CF_DIBV5           , "DIBV5"         	 },
	{ CF_MAX             , "MAX"           	 },
	{ CF_OWNERDISPLAY    , "OWNERDISPLAY"  	 },
	{ CF_DSPTEXT         , "DSPTEXT"       	 },
	{ CF_DSPBITMAP       , "DSPBITMAP"     	 },
	{ CF_DSPMETAFILEPICT , "DSPMETAFILEPICT" },
	{ CF_DSPENHMETAFILE  , "DSPENHMETAFILE"  },
};

std::string BaseHelper::ClipboardManager::GetFormat()
{
	JSON json;
	UINT format = 0;
	while ((format = EnumClipboardFormats(format)) != 0) {
		JSON j;
		j["key"] = format;
		auto it = sm_formats.find(format);
		if (it != sm_formats.end()) {
			j["name"] = it->second;
		}
		else {
			TCHAR szFormatName[1024];
			const int cchMaxCount = sizeof(szFormatName) / sizeof(TCHAR);
			int cActual = GetClipboardFormatName(format, szFormatName, cchMaxCount);
			if (cActual) j["name"] = WC2MB(szFormatName);
		}
		json.push_back(j);
	}
	return json.dump();
}

static std::vector<std::wstring> FileList()
{
	std::vector<std::wstring> result;
	if (HGLOBAL hGlobal = ::GetClipboardData(CF_HDROP)) {
		if (HDROP hDrop = (HDROP)GlobalLock(hGlobal)) {
			UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, 0, 0);
			for (UINT i = 0; i < fileCount; ++i) {
				UINT size = DragQueryFile(hDrop, i, 0, 0) + 1;
				std::vector<wchar_t> buffer(size);
				DragQueryFile(hDrop, i, buffer.data(), size);
				std::wstring filename = buffer.data();
				result.push_back(filename);
			}
			GlobalUnlock(hGlobal);
		}
	}
	return result;
}

std::wstring BaseHelper::ClipboardManager::GetText()
{
	if (!m_isOpened) return {};
	std::wstring result;
	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		if (HGLOBAL hGlobal = ::GetClipboardData(CF_UNICODETEXT)) {
			result = static_cast<LPWSTR>(GlobalLock(hGlobal));
			GlobalUnlock(hGlobal);
		}
	}
	else if (IsClipboardFormatAvailable(CF_TEXT)) {
		if (HGLOBAL hGlobal = ::GetClipboardData(CF_TEXT)) {
			result = MB2WC(static_cast<LPSTR>(GlobalLock(hGlobal)));
			GlobalUnlock(hGlobal);
		}
	}
	else if (IsClipboardFormatAvailable(CF_HDROP)) {
		for (auto& it : FileList()) {
			result.append(it).append(L"\r\n");
		}
	}

	return result;
}

std::wstring BaseHelper::ClipboardManager::GetFiles()
{
	JSON json;
	for (auto& it : FileList()) {
		json.push_back(WC2MB(it));
	}
	return json;
}

bool BaseHelper::ClipboardManager::SetText(const std::wstring& text, bool bEmpty)
{
	if (!m_isOpened) return false;
	if (bEmpty) EmptyClipboard();
	size_t size = (text.size() + 1) * sizeof(wchar_t);
	if (HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size)) {
		memcpy(GlobalLock(hGlobal), text.c_str(), size);
		GlobalUnlock(hGlobal);
		SetClipboardData(CF_UNICODETEXT, hGlobal);
		GlobalFree(hGlobal);
		return true;
	}
	else return false;
}

#include <shlobj.h> // DROPFILES

bool BaseHelper::ClipboardManager::SetFiles(const std::string &text, bool bEmpty)
{
	if (!m_isOpened) return false;
	if (bEmpty) EmptyClipboard();
	nlohmann::json json = nlohmann::json::parse(text);
	if (json.is_array()) {
		SIZE_T clpSize = sizeof(DROPFILES);
		for (auto element : json) {
			std::wstring filename = MB2WC(element);
			clpSize += (filename.size() + 1) * sizeof(TCHAR);
		}
		HDROP hDrop = (HDROP)GlobalAlloc(GHND, clpSize);
		DROPFILES* df = (DROPFILES*)GlobalLock(hDrop);
		df->pFiles = sizeof(DROPFILES); // string offset
		df->fWide = TRUE; // unicode file names

		// copy the command line args to the allocated memory
		TCHAR* dstStart = (TCHAR*)&df[1];
		for (auto element : json) {
			std::wstring filename = MB2WC(element);
			size_t len = filename.size() + 1;
			wmemcpy(dstStart, filename.c_str(), len);
			dstStart = &dstStart[len]; // + 1 => get beyond '\0'
		}
		GlobalUnlock(hDrop);
		SetClipboardData(CF_HDROP, hDrop);
	}
	return true;
}

bool BaseHelper::ClipboardManager::GetImage(VH variant)
{
	if (!m_isOpened) return false;

	static UINT CF_PNG = RegisterClipboardFormat(L"PNG");
	if (IsClipboardFormatAvailable(CF_PNG)) {
		HANDLE hData = ::GetClipboardData(CF_PNG);
		if (hData) {
			void* data = ::GlobalLock(hData);
			SIZE_T size = ::GlobalSize(hData);
			variant.AllocMemory(size);
			memcpy(variant.data(), data, size);
			::GlobalUnlock(hData);
			return true;
		}
	}

	HANDLE hData = ::GetClipboardData(CF_DIBV5);
	if (hData) {
		uint8_t* data = reinterpret_cast<uint8_t*>(::GlobalLock(hData));
		// CF_DIBV5 is composed of a BITMAPV5HEADER + bitmap data
		ImageHelper image(reinterpret_cast<BITMAPINFO*>(data), data + sizeof(BITMAPV5HEADER));
		if (image) image.Save(variant);
		::GlobalUnlock(hData);
	}
	return true;
}

bool BaseHelper::ClipboardManager::SetImage(VH variant, bool bEmpty)
{
	if (!m_isOpened) return false;

	ImageHelper image(variant);
	if (!image) return false;

	if (bEmpty) EmptyClipboard();

	std::vector<BYTE> vec;
	if (image.Save(vec)) {
		static UINT CF_PNG = RegisterClipboardFormat(L"PNG");
		if (auto hGlobal = GlobalAlloc(GMEM_MOVEABLE, vec.size())) {
			auto buffer = (BYTE*)GlobalLock(hGlobal);
			memcpy(buffer, vec.data(), vec.size());
			SetClipboardData(CF_PNG, hGlobal);
			GlobalUnlock(hGlobal);
			GlobalFree(hGlobal);
		}
		else return false;
	}
	else return false;

	HBITMAP hBitmap(image);
	if (hBitmap) {
		BITMAP bm;
		GetObject(hBitmap, sizeof bm, &bm);
		BITMAPINFOHEADER bi = { sizeof bi, bm.bmWidth, bm.bmHeight, 1, bm.bmBitsPixel, BI_RGB };
		std::vector<BYTE> vec(bm.bmWidthBytes * bm.bmHeight);
		auto hDC = GetDC(NULL);
		GetDIBits(hDC, hBitmap, 0, bi.biHeight, vec.data(), (BITMAPINFO*)&bi, 0);
		ReleaseDC(NULL, hDC);
		DeleteObject(hBitmap);

		if (auto hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeof bi + vec.size())) {
			auto buffer = (BYTE*)GlobalLock(hGlobal);
			memcpy(buffer, &bi, sizeof bi);
			memcpy(buffer + sizeof bi, vec.data(), vec.size());
			SetClipboardData(CF_DIB, hGlobal);
			GlobalUnlock(hGlobal);
			GlobalFree(hGlobal);
			return true;
		}
	}
	return false;
}

bool BaseHelper::ClipboardManager::Empty()
{
	return ::EmptyClipboard();
}

#else //_WINDOWS

#include "xcb_clip.h"

#include <vector>

namespace clip {
	namespace x11 {
		bool write_png(const clip::image& image, std::vector<uint8_t>& output);
		bool read_png(const uint8_t* buf, const size_t len, clip::image* output_image, clip::image_spec* output_spec);
	}
}

BaseHelper::ClipboardManager::ClipboardManager()
{
}

BaseHelper::ClipboardManager::~ClipboardManager()
{
}

bool BaseHelper::ClipboardManager::SetText(const std::wstring &text, bool bEmpty)
{
	return clip::set_text(WC2MB(text));
}

std::string BaseHelper::ClipboardManager::GetFormat()
{
  clip::lock l;
  if (l.locked()) {
	if (l.is_convertible(clip::text_format())) return "[{\"key\":1,\"name\":\"TEXT\"}]";
  	if (l.is_convertible(clip::image_format())) return "[{\"key\":2,\"name\":\"PNG\"}]";
  }
  return {};
}

std::wstring BaseHelper::ClipboardManager::GetText()
{
	std::string text;
	clip::get_text(text);
	return MB2WC(text);
}

bool BaseHelper::ClipboardManager::GetImage(VH data)
{
	clip::image image;
	clip::get_image(image);
	std::vector<uint8_t> buffer;
	clip::x11::write_png(image, buffer);
	if (buffer.empty()) return true;
	data.AllocMemory(buffer.size());
	memcpy(data.data(), buffer.data(), buffer.size());
	return true;
}

bool BaseHelper::ClipboardManager::SetImage(VH data, bool bEmpty)
{
	clip::image image;
	clip::x11::read_png((uint8_t*)data.data(), data.size(), &image, nullptr);
	return clip::set_image(image);
}

bool BaseHelper::ClipboardManager::Empty()
{
	clip::clear();
	return true;
}

std::wstring BaseHelper::ClipboardManager::GetFiles()
{
	return {};
}

bool BaseHelper::ClipboardManager::SetFiles(const std::string& text, bool bEmpty)
{
	return true;
}

#endif //_WINDOWS
