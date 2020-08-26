#include "stdafx.h"

#ifdef _WINDOWS
#include <locale>
#pragma warning (disable : 4267)
#pragma warning (disable : 4302)
#pragma warning (disable : 4311)
#else
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#endif

#include <wchar.h>
#include <string>
#include <algorithm>
#include <codecvt>
#include <cwctype>
#include <sstream>

#include "AddInNative.h"

#ifdef _WINDOWS
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#endif

const WCHAR_T* GetClassNames()
{
	static const std::u16string names(AddInNative::getComponentNames());
	return (const WCHAR_T*)names.c_str();
}

long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface)
{
	if (*pInterface) return 0;
	auto cls_name = std::u16string(reinterpret_cast<const char16_t*>(wsName));
	return long(*pInterface = AddInNative::CreateObject(cls_name));
}

long DestroyObject(IComponentBase** pInterface)
{
	if (!*pInterface) return -1;
	delete* pInterface;
	*pInterface = nullptr;
	return 0;
}

#ifdef _WINDOWS

std::string WC2MB(const std::wstring& wstr)
{
	DWORD locale = CP_UTF8;
	if (wstr.empty()) return {};
	const int sz = WideCharToMultiByte(locale, 0, &wstr[0], (int)wstr.size(), 0, 0, 0, 0);
	std::string res(sz, 0);
	WideCharToMultiByte(locale, 0, &wstr[0], (int)wstr.size(), &res[0], sz, 0, 0);
	return res;
}

std::wstring MB2WC(const std::string& str)
{
	DWORD locale = CP_UTF8;
	if (str.empty()) return {};
	const int sz = MultiByteToWideChar(locale, 0, &str[0], (int)str.size(), 0, 0);
	std::wstring res(sz, 0);
	MultiByteToWideChar(locale, 0, &str[0], (int)str.size(), &res[0], sz);
	return res;
}

#else //_WINDOWS

#include <iconv.h>

std::wstring MB2WC(const std::string& source)
{
	std::string tocode = sizeof(wchar_t) == 4 ? "UTF-32" : "UTF-16";
	iconv_t cd = iconv_open(tocode.c_str(), "UTF-8");
	if (cd == (iconv_t)-1) return {};

	std::wstring result;
	result.resize(source.size() + 1);

	char* src = const_cast<char*>(source.data());
	wchar_t* trg = const_cast<wchar_t*>(result.data());

	size_t succeed = (size_t)-1;
	size_t f = source.size() * sizeof(char);
	size_t t = result.size() * sizeof(wchar_t);
	succeed = iconv(cd, (char**)&src, &f, (char**)&trg, &t);
	iconv_close(cd);
	if (succeed == (size_t)-1) return {};
	return result;
}

std::string WC2MB(const std::wstring& source)
{
	std::string fromcode = sizeof(wchar_t) == 4 ? "UTF-32" : "UTF-16";
	iconv_t cd = iconv_open("UTF-8", fromcode.c_str());
	if (cd == (iconv_t)-1) return {};

	std::string result;
	result.resize(source.size() * sizeof(wchar_t) + 1);

	wchar_t* src = const_cast<wchar_t*>(source.data());
	char* trg = const_cast<char*>(result.data());

	size_t succeed = (size_t)-1;
	size_t f = source.size() * sizeof(wchar_t);
	size_t t = result.size() * sizeof(char);
	succeed = iconv(cd, (char**)&src, &f, (char**)&trg, &t);
	iconv_close(cd);
	if (succeed == (size_t)-1) return {};
	return result;
}

#endif //_WINDOWS

std::map<std::u16string, CompFunction> AddInNative::components;

bool AddInNative::Init(void* pConnection)
{
	m_iConnect = static_cast<IAddInDefBase*>(pConnection);
	if (m_iConnect) m_iConnect->SetEventBufferDepth(100);
	return m_iConnect != nullptr;
}

bool AddInNative::setMemManager(void* memory)
{
	return m_iMemory = static_cast<IMemoryManager*>(memory);
}

long AddInNative::GetInfo()
{
	return 2000;
}

void AddInNative::Done()
{
}

bool AddInNative::RegisterExtensionAs(WCHAR_T** wsLanguageExt)
{
	return *wsLanguageExt = W(name.c_str());
}

long AddInNative::GetNProps()
{
	return properties.size();
}

long AddInNative::FindProp(const WCHAR_T* wsPropName)
{
	std::u16string name((char16_t*)wsPropName);
	for (auto it = properties.begin(); it != properties.end(); it++) {
		for (auto n = it->names.begin(); n != it->names.end(); n++) {
			if (n->compare(name) == 0) return long(it - properties.begin());
		}
	}
	name = upper(name);
	for (auto it = properties.begin(); it != properties.end(); it++) {
		for (auto n = it->names.begin(); n != it->names.end(); n++) {
			if (upper(*n).compare(name) == 0) return long(it - properties.begin());
		}
	}
	return -1;
}

const WCHAR_T* AddInNative::GetPropName(long lPropNum, long lPropAlias)
{
	auto it = std::next(properties.begin(), lPropNum);
	if (it == properties.end()) return nullptr;
	auto nm = std::next(it->names.begin(), lPropAlias);
	if (nm == it->names.end()) return nullptr;
	return W(nm->c_str());
}

bool AddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	auto it = std::next(properties.begin(), lPropNum);
	if (it == properties.end()) return false;
	if (!it->getter) return false;
	try {
		it->getter(VA(pvarPropVal));
		return true;
	}
	catch (...) {
		return false;
	}
}

bool AddInNative::SetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	auto it = std::next(properties.begin(), lPropNum);
	if (it == properties.end()) return false;
	if (!it->setter) return false;
	try {
		it->setter(VA(pvarPropVal));
		return true;
	}
	catch (...) {
		return false;
	}
}

bool AddInNative::IsPropReadable(const long lPropNum)
{
	auto it = std::next(properties.begin(), lPropNum);
	if (it == properties.end()) return false;
	return (bool)it->getter;
}

bool AddInNative::IsPropWritable(const long lPropNum)
{
	auto it = std::next(properties.begin(), lPropNum);
	if (it == properties.end()) return false;
	return (bool)it->setter;
}

long AddInNative::GetNMethods()
{
	return methods.size();
}

long AddInNative::FindMethod(const WCHAR_T* wsMethodName)
{
	std::u16string name((char16_t*)wsMethodName);
	for (auto it = methods.begin(); it != methods.end(); it++) {
		for (auto n = it->names.begin(); n != it->names.end(); n++) {
			if (n->compare(name) == 0) return long(it - methods.begin());
		}
	}
	name = upper(name);
	for (auto it = methods.begin(); it != methods.end(); it++) {
		for (auto n = it->names.begin(); n != it->names.end(); n++) {
			if (upper(*n).compare(name) == 0) return long(it - methods.begin());
		}
	}
	return -1;
}

const WCHAR_T* AddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{
	auto it = std::next(methods.begin(), lMethodNum);
	if (it == methods.end()) return nullptr;
	auto nm = std::next(it->names.begin(), lMethodAlias);
	if (nm == it->names.end()) return nullptr;
	return W(nm->c_str());
}

long AddInNative::GetNParams(const long lMethodNum)
{
	auto it = std::next(methods.begin(), lMethodNum);
	if (it == methods.end()) return 0;
	if (std::get_if<MethFunction0>(&it->handler)) return 0;
	if (std::get_if<MethFunction1>(&it->handler)) return 1;
	if (std::get_if<MethFunction2>(&it->handler)) return 2;
	if (std::get_if<MethFunction3>(&it->handler)) return 3;
	if (std::get_if<MethFunction4>(&it->handler)) return 4;
	if (std::get_if<MethFunction5>(&it->handler)) return 5;
	if (std::get_if<MethFunction6>(&it->handler)) return 6;
	if (std::get_if<MethFunction7>(&it->handler)) return 7;
	return 0;
}

bool AddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue)
{
	try {
		VA(pvarParamDefValue).clear();
		auto it = std::next(methods.begin(), lMethodNum);
		if (it == methods.end()) return true;
		auto p = it->defs.find(lParamNum);
		if (p == it->defs.end()) return true;
		auto var = &p->second.variant;
		if (auto value = std::get_if<std::u16string>(var)) {
			VA(pvarParamDefValue) = *value;
			return true;
		}
		if (auto value = std::get_if<int64_t>(var)) {
			VA(pvarParamDefValue) = *value;
			return true;
		}
		if (auto value = std::get_if<double>(var)) {
			VA(pvarParamDefValue) = *value;
			return true;
		}
		if (auto value = std::get_if<bool>(var)) {
			VA(pvarParamDefValue) = *value;
			return true;
		}
		return true;
	}
	catch (...) {
		return false;
	}
}

bool AddInNative::HasRetVal(const long lMethodNum)
{
	auto it = std::next(methods.begin(), lMethodNum);
	if (it == methods.end()) return false;
	return it->hasRetVal;
}

bool AddInNative::CallMethod(MethFunction* function, tVariant* p, const long lSizeArray)
{
	if (auto handler = std::get_if<MethFunction0>(function)) {
		(*handler)();
		return true;
	}
	if (auto handler = std::get_if<MethFunction1>(function)) {
		if (lSizeArray < 1) throw std::bad_function_call();
		(*handler)(VA(p));
		return true;
	}
	if (auto handler = std::get_if<MethFunction2>(function)) {
		if (lSizeArray < 2) throw std::bad_function_call();
		(*handler)(VA(p), VA(p + 1));
		return true;
	}
	if (auto handler = std::get_if<MethFunction3>(function)) {
		if (lSizeArray < 3) throw std::bad_function_call();
		(*handler)(VA(p), VA(p + 1), VA(p + 2));
		return true;
	}
	if (auto handler = std::get_if<MethFunction4>(function)) {
		if (lSizeArray < 4) throw std::bad_function_call();
		(*handler)(VA(p), VA(p + 1), VA(p + 2), VA(p + 3));
		return true;
	}
	if (auto handler = std::get_if<MethFunction5>(function)) {
		if (lSizeArray < 5) throw std::bad_function_call();
		(*handler)(VA(p), VA(p + 1), VA(p + 2), VA(p + 3), VA(p + 4));
		return true;
	}
	if (auto handler = std::get_if<MethFunction6>(function)) {
		if (lSizeArray < 6) throw std::bad_function_call();
		(*handler)(VA(p), VA(p + 1), VA(p + 2), VA(p + 3), VA(p + 4), VA(p + 5));
		return true;
	}
	if (auto handler = std::get_if<MethFunction7>(function)) {
		if (lSizeArray < 7) throw std::bad_function_call();
		(*handler)(VA(p), VA(p + 1), VA(p + 2), VA(p + 3), VA(p + 4), VA(p + 5), VA(p + 6));
		return true;
	}
	return false;
}

bool AddInNative::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{
	auto it = std::next(methods.begin(), lMethodNum);
	if (it == methods.end()) return false;
	try {
		result << VA(nullptr);
		return CallMethod(&it->handler, paParams, lSizeArray);
	}
	catch (...) {
		return false;
	}
}

bool AddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	auto it = std::next(methods.begin(), lMethodNum);
	if (it == methods.end()) return false;
	try {
		result << VA(pvarRetValue);
		bool ok = CallMethod(&it->handler, paParams, lSizeArray);
		result << VA(nullptr);
		return ok;
	}
	catch (...) {
		result << VA(nullptr);
		return false;
	}
}

void AddInNative::SetLocale(const WCHAR_T* locale)
{
	try {
		std::locale::global(std::locale{ WCHAR2MB(locale) });
	}
	catch (std::runtime_error&) {
		std::locale::global(std::locale{ "" });
	}
}

std::u16string AddInNative::getComponentNames() {
	const char16_t* const delim = u"|";
	std::vector<std::u16string> names;
	for (auto it = components.begin(); it != components.end(); ++it) names.push_back(it->first);
	std::basic_ostringstream<char16_t, std::char_traits<char16_t>, std::allocator<char16_t>> imploded;
	std::copy(names.begin(), names.end(), std::ostream_iterator<std::u16string, char16_t, std::char_traits<char16_t>>(imploded, delim));
	std::u16string result = imploded.str();
	result.pop_back();
	return result;
}

std::u16string AddInNative::AddComponent(const std::u16string& name, CompFunction creator)
{
	components.insert({ name, creator });
	return name;
}

AddInNative* AddInNative::CreateObject(const std::u16string& name) {
	auto it = components.find(name);
	if (it == components.end()) return nullptr;
	AddInNative* object = it->second();
	object->name = name;
	return object;
}

void AddInNative::AddProperty(const std::u16string& nameEn, const std::u16string& nameRu, PropFunction getter, PropFunction setter)
{
	properties.push_back({ { nameRu, nameEn }, getter, setter });
}

void AddInNative::AddProcedure(const std::u16string& nameEn, const std::u16string& nameRu, MethFunction handler, MethDefaults defs)
{
	methods.push_back({ { nameRu, nameEn }, handler, defs, false });
}

void AddInNative::AddFunction(const std::u16string& nameEn, const std::u16string& nameRu, MethFunction handler, MethDefaults defs)
{
	methods.push_back({ { nameRu, nameEn }, handler, defs, true });
}

bool ADDIN_API AddInNative::AllocMemory(void** pMemory, unsigned long ulCountByte) const
{
	return m_iMemory ? m_iMemory->AllocMemory(pMemory, ulCountByte) : false;
}

void ADDIN_API AddInNative::FreeMemory(void** pMemory) const
{
	if (m_iMemory) m_iMemory->FreeMemory(pMemory);
}

std::string AddInNative::WCHAR2MB(std::basic_string_view<WCHAR_T> src)
{
	if (sizeof(wchar_t) == 2) {
		static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cvt_utf8_utf16;
		return cvt_utf8_utf16.to_bytes(src.data(), src.data() + src.size());
	}
	else {
		static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt_utf8_utf16;
		return cvt_utf8_utf16.to_bytes(reinterpret_cast<const char16_t*>(src.data()),
			reinterpret_cast<const char16_t*>(src.data() + src.size()));
	}
}

std::wstring AddInNative::WCHAR2WC(std::basic_string_view<WCHAR_T> src) {
	if (sizeof(wchar_t) == 2) {
		return std::wstring(src);
	}
	else {
		std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>> conv;
		return conv.from_bytes(reinterpret_cast<const char*>(src.data()),
			reinterpret_cast<const char*>(src.data() + src.size()));
	}
}

std::u16string AddInNative::MB2WCHAR(std::string_view src) {
	if (sizeof(wchar_t) == 2) {
		static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cvt_utf8_utf16;
		std::wstring tmp = cvt_utf8_utf16.from_bytes(src.data(), src.data() + src.size());
		return std::u16string(reinterpret_cast<const char16_t*>(tmp.data()), tmp.size());
	}
	else {
		static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt_utf8_utf16;
		return cvt_utf8_utf16.from_bytes(src.data(), src.data() + src.size());
	}
}

std::u16string AddInNative::upper(std::u16string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), std::towupper);
	return str;
}

std::wstring AddInNative::upper(std::wstring& str)
{
	std::transform(str.begin(), str.end(), str.begin(), std::towupper);
	return str;
}

uint32_t AddInNative::VarinantHelper::size()
{
	if (pvar == nullptr) throw std::bad_variant_access();
	if (pvar->vt != VTYPE_BLOB) throw std::bad_typeid();
	return pvar->strLen;
}

TYPEVAR AddInNative::VarinantHelper::type() 
{
	if (pvar == nullptr) throw std::bad_variant_access();
	return pvar->vt;
}

char* AddInNative::VarinantHelper::data()
{
	if (pvar == nullptr) throw std::bad_variant_access();
	if (pvar->vt != VTYPE_BLOB) throw std::bad_typeid();
	return pvar->pstrVal;
}

AddInNative::VarinantHelper& AddInNative::VarinantHelper::operator=(const std::string& str)
{
	return operator=(AddInNative::MB2WCHAR(str));
}

AddInNative::VarinantHelper& AddInNative::VarinantHelper::operator=(const std::wstring& str)
{
	if (sizeof(wchar_t) == 2) {
		return operator=(std::u16string(reinterpret_cast<const char16_t*>(str.data()), str.size()));
	}
	else {
		return operator=(WC2MB(str));
	}
}

void AddInNative::VarinantHelper::clear()
{
	if (pvar == nullptr) throw std::bad_variant_access();
	switch (TV_VT(pvar)) {
	case VTYPE_BLOB:
	case VTYPE_PWSTR:
		addin->FreeMemory(reinterpret_cast<void**>(&TV_WSTR(pvar)));
		break;
	}
	tVarInit(pvar);
}

AddInNative::VarinantHelper& AddInNative::VarinantHelper::operator=(int64_t value)
{
	clear();
	if (INT32_MIN <= value && value <= INT32_MAX) {
		TV_VT(pvar) = VTYPE_I4;
		TV_I4(pvar) = (int32_t)value;
	}
	else {
		TV_VT(pvar) = VTYPE_R8;
		TV_R8(pvar) = (double)value;
	}
	return *this;
}

AddInNative::VarinantHelper& AddInNative::VarinantHelper::operator=(double value)
{
	clear();
	TV_VT(pvar) = VTYPE_R8;
	TV_R8(pvar) = value;
	return *this;
}

AddInNative::VarinantHelper& AddInNative::VarinantHelper::operator=(bool value)
{
	clear();
	TV_VT(pvar) = VTYPE_BOOL;
	TV_BOOL(pvar) = value;
	return *this;
}

AddInNative::VarinantHelper& AddInNative::VarinantHelper::operator=(const std::u16string& str)
{
	clear();
	TV_VT(pvar) = VTYPE_PWSTR;
	pvar->pwstrVal = nullptr;
	size_t size = (str.size() + 1) * sizeof(char16_t);
	if (!addin->AllocMemory(reinterpret_cast<void**>(&pvar->pwstrVal), size)) throw std::bad_alloc();
	memcpy(pvar->pwstrVal, str.c_str(), size);
	pvar->wstrLen = str.size();
	while (pvar->wstrLen && pvar->pwstrVal[pvar->wstrLen - 1] == 0) pvar->wstrLen--;
	return *this;
}

AddInNative::VarinantHelper::operator std::string() const
{
	std::u16string str(*this);
	return WCHAR2MB((WCHAR_T*)str.c_str());
}

AddInNative::VarinantHelper::operator std::wstring() const
{
	std::u16string str(*this);
	return WCHAR2WC((WCHAR_T*)str.c_str());
}

AddInNative::VarinantHelper::operator std::u16string() const
{
	if (pvar == nullptr) throw std::bad_variant_access();
	if (pvar->vt != VTYPE_PWSTR) throw std::bad_typeid();
	return reinterpret_cast<char16_t*>(pvar->pwstrVal);
}

AddInNative::VarinantHelper::operator int64_t() const
{
	if (pvar == nullptr) throw std::bad_variant_access();
	switch (TV_VT(pvar)) {
	case VTYPE_I2:
	case VTYPE_I4:
	case VTYPE_UI1:
	case VTYPE_ERROR:
		return (int64_t)pvar->lVal;
	case VTYPE_R4:
	case VTYPE_R8:
		return (int64_t)pvar->dblVal;
	default:
		throw std::bad_typeid();
	}
}

AddInNative::VarinantHelper::operator double() const
{
	if (pvar == nullptr) throw std::bad_variant_access();
	switch (TV_VT(pvar)) {
	case VTYPE_I2:
	case VTYPE_I4:
	case VTYPE_UI1:
	case VTYPE_ERROR:
		return (double)pvar->lVal;
	case VTYPE_R4:
	case VTYPE_R8:
		return (double)pvar->dblVal;
	default:
		throw std::bad_typeid();
	}
}

AddInNative::VarinantHelper::operator bool() const
{
	if (pvar == nullptr) throw std::bad_variant_access();
	if (TV_VT(pvar) != VTYPE_BOOL) throw std::bad_typeid();
	return TV_BOOL(pvar);
}

void AddInNative::VarinantHelper::AllocMemory(unsigned long size)
{
	clear();
	if (!addin->AllocMemory((void**)&pvar->pstrVal, size)) throw std::bad_alloc();
	TV_VT(pvar) = VTYPE_BLOB;
	pvar->strLen = size;
}

WCHAR_T* AddInNative::W(const char16_t* str) const
{
	WCHAR_T* res = NULL;
	size_t length = std::char_traits<char16_t>::length(str) + 1;
	unsigned long size = length * sizeof(WCHAR_T);
	if (!AllocMemory((void**)&res, size)) throw std::bad_alloc();
	memcpy(res, str, size);
	return res;
}

