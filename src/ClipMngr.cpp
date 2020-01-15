#include "ClipMngr.h"

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
