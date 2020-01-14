#include "ClipMngr.h"

#ifdef _WINDOWS

#include "ImageHelper.h"

std::wstring ClipboardManager::GetText()
{
	std::wstring result;
	if (OpenClipboard(nullptr)) {
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
		CloseClipboard();
	}
	return result;
}

bool ClipboardManager::SetText(const std::wstring& text)
{
	OpenClipboard(nullptr);
	EmptyClipboard();
	size_t size = (text.size() + 1) * sizeof(wchar_t);
	if (HGLOBAL hglobal = GlobalAlloc(GMEM_MOVEABLE, size)) {
		memcpy(GlobalLock(hglobal), text.c_str(), size);
		GlobalUnlock(hglobal);
		SetClipboardData(CF_UNICODETEXT, hglobal);
		GlobalFree(hglobal);
	}
	CloseClipboard();
	return true;
}

bool ClipboardManager::GetImage(tVariant* pvarRetValue)
{
	if (!::OpenClipboard(NULL)) return false;
	HANDLE hData = ::GetClipboardData(CF_DIBV5);
	if (hData) {
		uint8_t* data = reinterpret_cast<uint8_t*>(::GlobalLock(hData));
		// CF_DIBV5 is composed of a BITMAPV5HEADER + bitmap data
		ImageHelper image(reinterpret_cast<BITMAPINFO*>(data), data + sizeof(BITMAPV5HEADER));
		if (image) image.Save(m_iMemory, pvarRetValue);
		::GlobalUnlock(hData);
	}
	::CloseClipboard();
	return true;
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

#endif //_WINDOWS
