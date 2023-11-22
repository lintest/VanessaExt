#pragma once
#include "stdafx.h"
#include "json.hpp"

class FileFinder {
private:
	std::wstring m_text;
	bool m_ignoreCase;
	bool m_recurseDirs;
	bool m_includeDirs;
	nlohmann::json m_json;
public:
	FileFinder(const std::wstring& text, bool ignoreCase, bool recurseDirs, bool includeDirs);
	std::wstring find(const std::wstring& path, const std::wstring& mask);
	bool search(const std::wstring& path);
private:
	void dirs(const std::wstring& root, const std::wstring& mask);
	void files(const std::wstring& root, const std::wstring& mask);
};
