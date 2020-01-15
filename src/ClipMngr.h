#ifndef __CLIPMNGR_H__
#define __CLIPMNGR_H__

#include "stdafx.h"
#include <map>

class ClipboardManager {
public:
	ClipboardManager(IMemoryManager* iMemory);
	virtual ~ClipboardManager();
	std::wstring GetFormat();
	bool SetText(const std::wstring& text);
	std::wstring GetText();
	bool GetImage(tVariant* pvarValue);
	bool SetImage(tVariant* pvarValue);
	bool Empty();
private:
	const std::string StdName(UINT format) const;
	static const std::map<int, std::string> sm_formats;
	IMemoryManager* m_iMemory = 0;
	bool m_isOpened = false;
};

#endif //__CLIPMNGR_H__
