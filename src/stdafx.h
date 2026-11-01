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

extern std::locale locale_ru;

// Смена регистра для BMP без зависимости от системной локали: в контейнерах
// локаль ru_RU.UTF-8 часто отсутствует, а std::tolower с такой локалью бросает
// исключение уже во время работы (фасет создаётся лениво). Эти функции — аварийный
// путь, когда locale_ru недоступна.
inline char16_t toUpperRu(char16_t ch)
{
	if (ch >= u'а' && ch <= u'я') return char16_t(ch - 0x20);
	if (ch == u'ё') return u'Ё';
	if (ch >= u'a' && ch <= u'z') return char16_t(ch - 0x20);
	return ch;
}

inline char16_t toLowerRu(char16_t ch)
{
	if (ch >= u'А' && ch <= u'Я') return char16_t(ch + 0x20);
	if (ch == u'Ё') return u'ё';
	if (ch >= u'A' && ch <= u'Z') return char16_t(ch + 0x20);
	return ch;
}
