#ifndef __CLIPMNGR_H__
#define __CLIPMNGR_H__

#include "stdafx.h"

class ClipboardManager {
public:
	ClipboardManager(IMemoryManager* iMemory);
	virtual ~ClipboardManager();
	bool SetText(const std::wstring& text);
	std::wstring GetText();
	bool GetImage(tVariant* pvarValue);
	bool SetImage(tVariant* pvarValue);
	bool Empty();
private:
	IMemoryManager* m_iMemory = 0;
	bool m_isOpened = false;
};

#endif //__CLIPMNGR_H__
