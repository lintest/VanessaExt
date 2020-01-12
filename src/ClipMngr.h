#ifndef __CLIPMNGR_H__
#define __CLIPMNGR_H__

#include "stdafx.h"

class ClipboardManager {
public:
	ClipboardManager(IMemoryManager* iMemory) { m_iMemory = iMemory; }
	static bool SetText(const std::wstring& text);
	static std::wstring GetText();
	bool GetImage(tVariant* pvarRetValue);
private:
	IMemoryManager* m_iMemory;
};

#endif //__CLIPMNGR_H__
