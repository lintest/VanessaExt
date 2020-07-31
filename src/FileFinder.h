#ifndef __FILEFINDER_H__
#define __FILEFINDER_H__

#include "stdafx.h"
#include "json.hpp"

class FileFinder {
private:
	std::wstring m_text;
	bool m_ignoreCase;
	nlohmann::json m_json;
public:
	FileFinder(const std::wstring& text, bool ignoreCase);
	std::wstring find(const std::wstring& path, const std::wstring& mask);
private:
	void dirs(const std::wstring& root, const std::wstring& mask);
	void files(const std::wstring& root, const std::wstring& mask);
	bool search(const std::wstring& path);
};

#endif __FILEFINDER_H__
