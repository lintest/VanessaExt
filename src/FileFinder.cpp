#include "FileFinder.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <codecvt>

static void wstr_tolower(std::wstring& text)
{
	std::use_facet<std::ctype<wchar_t> >(std::locale()).tolower(&text[0], &text[0] + text.size());
}

FileFinder::FileFinder(const std::wstring& text, bool ignoreCase, bool recurseDirs, bool includeDirs)
	: m_text(text), m_ignoreCase(ignoreCase), m_recurseDirs(recurseDirs), m_includeDirs(includeDirs)
{
	if (ignoreCase) {
		wstr_tolower(m_text);
	}
}

bool FileFinder::search(const std::wstring& path)
{
	if (m_text.empty()) {
		return true;
	}
#ifdef _WINDOWS
	std::wifstream wif(path);
#else
	std::wifstream wif(WC2MB(path));
#endif
	static std::locale locale_ru = std::locale("ru_RU.UTF-8");
	wif.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
	const size_t text_len = m_text.length();
	const size_t buf_size = 8192;
	wchar_t buf[buf_size];
	size_t offset = 0;
	while (true) {
		wif.read(buf + offset, buf_size - offset);
		auto read = wif.gcount();
		if (read == 0) return false;
		wchar_t* end = buf + offset + read;
		if (m_ignoreCase) std::use_facet<std::ctype<wchar_t> >(locale_ru).tolower(buf + offset, end);
		auto it = std::search(buf, end, m_text.begin(), m_text.end());
		if (it != end) return true;
		offset = text_len;
		std::memmove(buf, end - offset, offset * sizeof(wchar_t));
	}
}

#ifdef _WINDOWS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

std::string time2str(FILETIME& time) {
	SYSTEMTIME st;
	FileTimeToSystemTime(&time, &st);
	char* format = "%d-%02d-%02dT%02d:%02d:%02dZ";
	char buffer[255];
	wsprintfA(buffer,
		format,
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond);
	return buffer;
}

void FileFinder::files(const std::wstring& root, const std::wstring& mask)
{
	std::wstring tmp = root + L"\\" + mask;
	WIN32_FIND_DATA file;
	HANDLE hFind = FindFirstFileEx(tmp.c_str(), FindExInfoStandard, &file, FindExSearchNameMatch, NULL, 0);
	if (hFind == INVALID_HANDLE_VALUE) return;
	do {
		std::wstring path = root + L"\\" + std::wstring(file.cFileName);
		if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (lstrcmpW(file.cFileName, L".") == 0) continue;
			if (lstrcmpW(file.cFileName, L"..") == 0) continue;
			if (m_includeDirs) {
				nlohmann::json j;
				j["path"] = WC2MB(path);
				j["name"] = WC2MB(file.cFileName);
				j["date"] = time2str(file.ftLastWriteTime);
				j["dir"] = true;
				m_json.push_back(j);
			}
		}
		else {
			if (search(path)) {
				nlohmann::json j;
				j["path"] = WC2MB(path);
				j["name"] = WC2MB(file.cFileName);
				j["size"] = (static_cast<ULONGLONG>(file.nFileSizeHigh) << sizeof(file.nFileSizeLow) * 8) | file.nFileSizeLow;
				j["date"] = time2str(file.ftLastWriteTime);
				m_json.push_back(j);
			}
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
			if (m_recurseDirs) {
				dirs(path, mask);
			}
		}
	} while (FindNextFileW(hFind, &file));
	FindClose(hFind);
}

std::wstring FileFinder::find(const std::wstring& path, const std::wstring& mask)
{
	dirs(path, mask);
	return MB2WC(m_json.dump());
}

#else //_WINDOWS

#ifdef USE_BOOST

#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>

static void EscapeRegex(std::wstring& regex)
{
	// Escape all regex special chars
	boost::replace_all(regex, "\\", "\\\\");
	boost::replace_all(regex, "^", "\\^");
	boost::replace_all(regex, ".", "\\.");
	boost::replace_all(regex, "$", "\\$");
	boost::replace_all(regex, "|", "\\|");
	boost::replace_all(regex, "(", "\\(");
	boost::replace_all(regex, ")", "\\)");
	boost::replace_all(regex, "{", "\\{");
	boost::replace_all(regex, "{", "\\}");
	boost::replace_all(regex, "[", "\\[");
	boost::replace_all(regex, "]", "\\]");
	boost::replace_all(regex, "+", "\\+");
	boost::replace_all(regex, "?", "\\?");
	boost::replace_all(regex, "/", "\\/");

	// Convert chars '*?' back to their regex equivalents
	boost::replace_all(regex, "\\?", ".");
	boost::replace_all(regex, "\\*", ".*");
}

#include <unistd.h>
#include <sys/stat.h>

static std::string time2str(time_t t)
{
	struct tm lt;
	localtime_r(&t, &lt);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%FT%T", &lt);
	return buffer;
}

template<typename T>
class FileIterator {
private:
	FileFinder& m_owner;
	JSON& m_json;
	bool m_includeDirs;
public:
	FileIterator(FileFinder& owner, JSON& json, bool includeDirs)
		: m_owner(owner), m_json(json), m_includeDirs(includeDirs) {}
	void find(const std::wstring& root, boost::wregex& pattern) {
		T end_itr; // Default ctor yields past-the-end
		for (T i(root); i != end_itr; ++i) {
			if (boost::filesystem::is_regular_file(i->status())) {
				boost::wsmatch what;
				std::wstring filepath = i->path().wstring();
				std::wstring filename = i->path().filename().wstring();
				if (!boost::regex_match(filename, what, pattern)) continue;
				if (m_owner.search(i->path().wstring())) {
					nlohmann::json j;
					j["path"] = WC2MB(filepath);
					j["name"] = WC2MB(filename);
					j["size"] = (int32_t)boost::filesystem::file_size(filepath);
					j["date"] = time2str(boost::filesystem::last_write_time(filepath));
					m_json.push_back(j);
				}
			}
			else if (m_includeDirs && boost::filesystem::is_directory(i->status())) {
				boost::wsmatch what;
				std::wstring filepath = i->path().wstring();
				std::wstring filename = i->path().filename().wstring();
				if (!boost::regex_match(filename, what, pattern)) continue;
				nlohmann::json j;
				j["path"] = WC2MB(filepath);
				j["name"] = WC2MB(filename);
				j["date"] = time2str(boost::filesystem::last_write_time(filepath));
				j["dir"] = true;
				m_json.push_back(j);
			}
		}
	}
};

void FileFinder::dirs(const std::wstring& root, const std::wstring& mask)
{
	boost::wregex pattern(mask, m_ignoreCase ? boost::regex::icase : boost::regex::normal);
	if (m_recurseDirs) {
		FileIterator<boost::filesystem::recursive_directory_iterator>(*this, m_json, m_includeDirs).find(root, pattern);
	}
	else {
		FileIterator<boost::filesystem::directory_iterator>(*this, m_json, m_includeDirs).find(root, pattern);
	}
}

std::wstring FileFinder::find(const std::wstring& path, const std::wstring& mask)
{
	std::wstring regex = mask;
	EscapeRegex(regex);
	dirs(path, L"^" + regex + L"$");
	return MB2WC(m_json.dump());
}

#else //USE_BOOST

std::wstring FileFinder::find(const std::wstring& path, const std::wstring& mask) { return {}; }

void FileFinder::dirs(const std::wstring& root, const std::wstring& mask) {}

#endif //USE_BOOST

void FileFinder::files(const std::wstring& root, const std::wstring& mask) {}

#endif //_WINDOWS
