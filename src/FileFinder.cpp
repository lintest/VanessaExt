#include "FileFinder.h"
#include "json_ext.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <codecvt>

static void lower(std::wstring& text)
{
	std::use_facet<std::ctype<wchar_t> >(std::locale()).tolower(&text[0], &text[0] + text.size());
}

FileFinder::FileFinder(const std::wstring& text, bool ignoreCase) 
	: m_text(text), m_ignoreCase(ignoreCase)
{
	if (ignoreCase) lower(m_text);
}

static std::wstring read(const std::wstring& filename)
{
	std::wifstream wif(filename);
	wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
	std::wstringstream wss;
	wss << wif.rdbuf();
	return wss.str();
}

bool FileFinder::search(const std::wstring& path)
{
	std::wstring file = read(path);
	if (m_ignoreCase) lower(file);
	auto it = std::search(file.begin(), file.end(), m_text.begin(), m_text.end());
	return it != file.end();
}

#ifdef _WINDOWS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

std::string time2str(FILETIME &time) {
	SYSTEMTIME st;
	FileTimeToSystemTime(&time, &st);
	char* format = "%d-%02d-%02dT%02d:%02d:%02d.%03dZ";
	char buffer[255];
	wsprintfA(buffer, 
		format,
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		st.wMilliseconds);
	return buffer;
}

void FileFinder::files(const std::wstring& root, const std::wstring& mask)
{
	std::wstring tmp = root + L"\\" + mask;
	WIN32_FIND_DATA file;
	HANDLE hFind = FindFirstFileEx(tmp.c_str(), FindExInfoStandard, &file, FindExSearchNameMatch, NULL, 0);
	if (hFind == INVALID_HANDLE_VALUE) return;
	do {
		if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		std::wstring path = root + L"\\" + std::wstring(file.cFileName);
		if (search(path)) {
			nlohmann::json j;
			j["path"] = WC2MB(path);
			j["name"] = WC2MB(file.cFileName);
			j["size"] = (static_cast<ULONGLONG>(file.nFileSizeHigh) << sizeof(file.nFileSizeLow) * 8) | file.nFileSizeLow;
			j["date"] = time2str(file.ftLastWriteTime);
			m_json.push_back(j);
		}
	} while (FindNextFileW(hFind, &file));
	FindClose(hFind);
}

void FileFinder::dirs(const std::wstring& root, const std::wstring& mask)
{
	files(root, mask);
	std::wstring tmp = root + L"\\*";
	WIN32_FIND_DATA file;
	HANDLE hFind = FindFirstFileEx(tmp.c_str(), FindExInfoStandard, &file, FindExSearchNameMatch, NULL, 0);
	if (hFind == INVALID_HANDLE_VALUE) return;
	do {
		if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (lstrcmpW(file.cFileName, L".") == 0) continue;
			if (lstrcmpW(file.cFileName, L"..") == 0) continue;
			std::wstring path = root + L"\\" + std::wstring(file.cFileName);
			dirs(path, mask);
		}
	} while (FindNextFileW(hFind, &file));
	FindClose(hFind);
}

#else

#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

int find()
{
	std::wstring path = L"C:\\Cpp\\SmartEngine";
	const boost::regex my_filter{ "/*\\.exe/" };

	boost::filesystem::directory_iterator end_itr; // Default ctor yields past-the-end
	for (boost::filesystem::directory_iterator i(path); i != end_itr; ++i)
	{
		if (!boost::filesystem::is_regular_file(i->status())) continue;
		boost::smatch what;
		if (!boost::regex_match(i->path().filename().string(), what, my_filter)) continue;
		std::wcout << i->path() << std::endl;
	}
}

#endif //_WINDOWS

std::wstring FileFinder::find(const std::wstring& path, const std::wstring& mask)
{
	dirs(path, mask);
	return MB2WC(m_json.dump());
}
