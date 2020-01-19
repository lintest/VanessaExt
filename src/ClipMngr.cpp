#include "ClipMngr.h"
#include "json_ext.h"

#ifdef _WINDOWS

#include "ImageHelper.h"

ClipboardManager::ClipboardManager(AddInNative* addin) : m_addin(addin)
{
	m_isOpened = OpenClipboard(nullptr);
}

ClipboardManager::~ClipboardManager()
{
	if (m_isOpened) CloseClipboard();
}

const std::map<int, std::string> ClipboardManager::sm_formats{
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

std::wstring ClipboardManager::GetFormat()
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
	return json;
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

std::wstring ClipboardManager::GetText()
{
	if (!m_isOpened) return {};
	std::wstring result;
	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		if (HGLOBAL hglobal = ::GetClipboardData(CF_UNICODETEXT)) {
			result = static_cast<LPWSTR>(GlobalLock(hglobal));
			GlobalUnlock(hglobal);
		}
	}
	else if (IsClipboardFormatAvailable(CF_TEXT)) {
		if (HGLOBAL hglobal = ::GetClipboardData(CF_TEXT)) {
			result = MB2WC(static_cast<LPSTR>(GlobalLock(hglobal)));
			GlobalUnlock(hglobal);
		}
	}
	else if (IsClipboardFormatAvailable(CF_HDROP)) {
		for (auto& it : FileList()) {
			result.append(it).append(L"\r\n");
		}
	}

	return result;
}

std::wstring ClipboardManager::GetFiles()
{
	JSON json;
	for (auto& it : FileList()) {
		json.push_back(WC2MB(it));
	}
	return json;
}

bool ClipboardManager::SetText(tVariant* pvarValue, bool bEmpty)
{
	if (!m_isOpened) return false;
	if (bEmpty) EmptyClipboard();
	std::wstring text = VarToStr(pvarValue);
	size_t size = (text.size() + 1) * sizeof(wchar_t);
	if (HGLOBAL hglobal = GlobalAlloc(GMEM_MOVEABLE, size)) {
		memcpy(GlobalLock(hglobal), text.c_str(), size);
		GlobalUnlock(hglobal);
		SetClipboardData(CF_UNICODETEXT, hglobal);
		GlobalFree(hglobal);
	}
	return true;
}

bool ClipboardManager::GetImage(tVariant* pvarValue)
{
	if (!m_isOpened) return false;
	HANDLE hData = ::GetClipboardData(CF_DIBV5);
	if (hData && m_addin) {
		uint8_t* data = reinterpret_cast<uint8_t*>(::GlobalLock(hData));
		// CF_DIBV5 is composed of a BITMAPV5HEADER + bitmap data
		ImageHelper image(reinterpret_cast<BITMAPINFO*>(data), data + sizeof(BITMAPV5HEADER));
		if (image) image.Save(m_addin, pvarValue);
		::GlobalUnlock(hData);
	}
	return true;
}

bool ClipboardManager::SetImage(tVariant* paParams, bool bEmpty)
{
	if (!m_isOpened) return false;

	ImageHelper image(paParams);
	if (!image) return false;

	HBITMAP hbitmap(image);
	if (!hbitmap) return false;

	BITMAP bm;
	GetObject(hbitmap, sizeof bm, &bm);

	BITMAPINFOHEADER bi =
	{ sizeof bi, bm.bmWidth, bm.bmHeight, 1, bm.bmBitsPixel, BI_RGB };

	std::vector<BYTE> vec(bm.bmWidthBytes * bm.bmHeight);
	auto hdc = GetDC(NULL);
	GetDIBits(hdc, hbitmap, 0, bi.biHeight, vec.data(), (BITMAPINFO*)&bi, 0);
	ReleaseDC(NULL, hdc);
	DeleteObject(hbitmap);

	auto hmem = GlobalAlloc(GMEM_MOVEABLE, sizeof bi + vec.size());
	auto buffer = (BYTE*)GlobalLock(hmem);
	memcpy(buffer, &bi, sizeof bi);
	memcpy(buffer + sizeof bi, vec.data(), vec.size());
	GlobalUnlock(hmem);

	if (bEmpty) EmptyClipboard();
	SetClipboardData(CF_DIB, hmem);
	GlobalFree(hmem);

	return true;
}

bool ClipboardManager::Empty()
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

ClipboardManager::ClipboardManager(AddInNative* addin) : m_addin(addin)
{
}

ClipboardManager::~ClipboardManager()
{
}

bool ClipboardManager::SetText(tVariant* pvarValue, bool bEmpty)
{
	std::wstring text = VarToStr(pvarValue);
	clip::set_text(WC2MB(text));
	return true;
}

std::wstring ClipboardManager::GetText()
{
	std::string text;
	clip::get_text(text);
	return MB2WC(text);
}

#include <iostream>
#include <cstdio>

bool ClipboardManager::GetImage(tVariant* pvarRetValue)
{
	clip::image image;
	clip::get_image(image);
	std::vector<uint8_t> buffer;
	clip::x11::write_png(image, buffer);
	if (buffer.empty()) return true;
	m_addin->AllocMemory((void**)&pvarRetValue->pstrVal, buffer.size());
	memcpy(pvarRetValue->pstrVal, buffer.data(), buffer.size());
	pvarRetValue->strLen = buffer.size();
	TV_VT(pvarRetValue) = VTYPE_BLOB;
	return true;
}

bool ClipboardManager::SetImage(tVariant* paParams, bool bEmpty)
{
	clip::image image;
	clip::x11::read_png((uint8_t*)paParams->pstrVal, paParams->strLen, &image, nullptr);
	clip::set_image(image);
	return true;
}

bool ClipboardManager::Empty()
{
	clip::clear();
}

std::wstring ClipboardManager::GetFiles()
{
	return {};
}

#endif //_WINDOWS
