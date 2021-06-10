#pragma once
#include <../include/types.h>
#include <string>
#include <memory>

#ifdef _WINDOWS

extern HMODULE hModule;

template<class T>
struct ComDeleter {
	void operator()(T* a) { if (a) a->Release(); }
};

template<class T, class D = ComDeleter<T>>
using ComUniquePtr = std::unique_ptr<T, D>;

template<class T, class D = ComDeleter<T>>
class DComPtr {
private:
	T* ptr = nullptr;
	ComUniquePtr<T, D>& src;
public:
	DComPtr(ComUniquePtr<T, D>& p) : ptr(p.get()), src(p) { }
	virtual ~DComPtr() { src.reset(ptr); }
	T** operator&() { return &ptr; }
	operator T** () { return &ptr; }
	operator T* () { return ptr; }
};

#endif //_WINDOWS

std::wstring MB2WC(const std::string& source);
std::string WC2MB(const std::wstring& source);
std::u16string MB2WCHAR(std::string_view src);
std::string WCHAR2MB(std::basic_string_view<WCHAR_T> src);
std::wstring WCHAR2WC(std::basic_string_view<WCHAR_T> src);

std::u16string upper(const std::u16string& str);
std::u16string lower(const std::u16string& str);
std::wstring upper(const std::wstring& str);
std::wstring lower(const std::wstring& str);

std::string cp1251_to_utf8(const char* str);

#include "json.hpp"
using JSON = nlohmann::json;

constexpr auto PROCESS_FINISHED = u"PROCESS_FINISHED";
