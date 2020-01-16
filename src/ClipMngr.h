#ifndef __CLIPMNGR_H__
#define __CLIPMNGR_H__

#include "stdafx.h"
#include <map>

class ClipboardManager {
public:
	ClipboardManager(AddInNative* addin);
	virtual ~ClipboardManager();
	std::wstring GetFormat();
	bool SetText(const std::wstring& text);
	std::wstring GetText();
	std::wstring GetFiles();
	bool GetImage(tVariant* pvarValue);
	bool SetImage(tVariant* pvarValue);
	bool Empty();
private:
	static const std::map<int, std::string> sm_formats;
	AddInNative* m_addin = nullptr;
	bool m_isOpened = false;
};

#endif //__CLIPMNGR_H__
