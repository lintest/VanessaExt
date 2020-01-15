#include "ClipMngr.h"
#include "json_ext.h"

#ifdef _WINDOWS

#include "ImageHelper.h"

ClipboardManager::ClipboardManager(IMemoryManager* iMemory)
{
	m_isOpened = OpenClipboard(nullptr);
	m_iMemory = iMemory;
}

ClipboardManager::~ClipboardManager()
{
	if (m_isOpened) CloseClipboard();
}

static std::string StandartFormat(UINT format)
{
	switch (format) {
	case CF_TEXT:
		return "TEXT";
	case CF_BITMAP:
		return "BITMAP";
	case CF_METAFILEPICT:
		return "METAFILEPICT";
	case CF_SYLK:
		return "SYLK";
	case CF_DIF:
		return "DIF";
	case CF_TIFF:
		return "TIFF";
	case CF_OEMTEXT:
		return "OEMTEXT";
	case CF_DIB:
		return "DIB";
	case CF_PALETTE:
		return "PALETTE";
	case CF_PENDATA:
		return "PENDATA";
	case CF_RIFF:
		return "RIFF";
	case CF_WAVE:
		return "WAVE";
	case CF_UNICODETEXT:
		return "UNICODETEXT";
	case CF_ENHMETAFILE:
		return "ENHMETAFILE";
	case CF_HDROP:
		return "HDROP";
	case CF_LOCALE:
		return "LOCALE";
	case CF_DIBV5:
		return "DIBV5";
	case CF_MAX:
		return "MAX";
	case CF_OWNERDISPLAY:
		return "OWNERDISPLAY";
	case CF_DSPTEXT:
		return "DSPTEXT";
	case CF_DSPBITMAP:
		return "DSPBITMAP";
	case CF_DSPMETAFILEPICT:
		return "DSPMETAFILEPICT";
	case CF_DSPENHMETAFILE:
		return "DSPENHMETAFILE";
	default:
		return {};
	};
}

std::wstring ClipboardManager::GetFormat()
{
	JSON json;
	UINT format = 0;
	while ((format = EnumClipboardFormats(format)) != 0) {
		std::string name = StandartFormat(format);
		if (!name.empty()) {
			json.push_back(name);
			continue;
		}
		TCHAR szFormatName[1024];
		const int cchMaxCount = sizeof(szFormatName) / sizeof(TCHAR);
		int cActual = GetClipboardFormatName(format, szFormatName, cchMaxCount);
		if (cActual) json.push_back(WC2MB(szFormatName));
	}
	return json;
}

std::wstring ClipboardManager::GetText()
{
	if (!m_isOpened) return {};
	std::wstring result;
	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		if (HGLOBAL hglobal = GetClipboardData(CF_UNICODETEXT)) {
			result = static_cast<LPWSTR>(GlobalLock(hglobal));
			GlobalUnlock(hglobal);
		}
	}
	else if (IsClipboardFormatAvailable(CF_TEXT)) {
		if (HGLOBAL hglobal = GetClipboardData(CF_TEXT)) {
			result = MB2WC(static_cast<LPSTR>(GlobalLock(hglobal)));
			GlobalUnlock(hglobal);
		}
	}
	return result;
}

bool ClipboardManager::SetText(const std::wstring& text)
{
	if (!m_isOpened) return false;
	EmptyClipboard();
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
	if (hData && m_iMemory) {
		uint8_t* data = reinterpret_cast<uint8_t*>(::GlobalLock(hData));
		// CF_DIBV5 is composed of a BITMAPV5HEADER + bitmap data
		ImageHelper image(reinterpret_cast<BITMAPINFO*>(data), data + sizeof(BITMAPV5HEADER));
		if (image) image.Save(m_iMemory, pvarValue);
		::GlobalUnlock(hData);
	}
	return true;
}

bool ClipboardManager::SetImage(tVariant* pvarValue)
{
	if (!m_isOpened) return false;

	ImageHelper image(pvarValue);
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

	auto hmem = GlobalAlloc(GMEM_MOVEABLE, sizeof bi + vec.size());
	auto buffer = (BYTE*)GlobalLock(hmem);
	memcpy(buffer, &bi, sizeof bi);
	memcpy(buffer + sizeof bi, vec.data(), vec.size());
	GlobalUnlock(hmem);

	EmptyClipboard();
	SetClipboardData(CF_DIB, hmem);
	DeleteObject(hbitmap);
	return true;
}

bool ClipboardManager::Empty()
{
	return ::EmptyClipboard();
}

#else //_WINDOWS

bool ClipboardManager::SetText(const std::wstring& text)
{
	return false;
}

std::wstring ClipboardManager::GetText()
{
	return {};
}

bool ClipboardManager::GetImage(tVariant* pvarRetValue)
{
	return false;
}

bool ClipboardManager::SetImage(tVariant* pvarValue)
{
	return false;
}

bool ClipboardManager::Empty()
{
	return false;
}

#endif //_WINDOWS
