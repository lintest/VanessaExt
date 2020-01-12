#include "ClipMngr.h"

#ifdef _WINDOWS

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
	return false;
}

#else //_WINDOWS



#endif //_WINDOWS