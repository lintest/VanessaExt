#ifndef __CLIPMNGR_H__
#define __CLIPMNGR_H__

#include "AddInNative.h"
#include <map>

class ClipboardManager
	: public AddInNative 
{
public:
	ClipboardManager();
	virtual ~ClipboardManager();
	std::wstring GetFormat();
	std::wstring GetText();
	std::wstring GetFiles();
	bool GetImage(VH data);
	bool SetImage(VH data, bool bEmpty = true);
	bool SetText(const std::wstring &text, bool bEmpty = true);
	bool SetFiles(const std::string& text, bool bEmpty = true);
	bool Empty();
private:
	static const std::map<int, std::string> sm_formats;
	bool m_isOpened = false;
};

#endif //__CLIPMNGR_H__
