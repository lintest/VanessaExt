#include "stdafx.h"

#ifdef _WINDOWS
#pragma setlocale("ru-RU" )
#else //_WINDOWS
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <iconv.h>
#include <sys/time.h>
#endif //_WINDOWS

#include "ClipboardControl.h"
#include "version.h"

#include "ClipMngr.h"

const wchar_t* ClipboardControl::m_ExtensionName = L"ClipboardControl";

const std::vector<AddInBase::Alias> ClipboardControl::m_PropList{
	Alias(eText    , true  , L"Text"    , L"Текст"),
	Alias(eImage   , true  , L"Image"   , L"Картинка"),
	Alias(eFormat  , false , L"Format"  , L"Формат"),
	Alias(eVersion , false , L"Version" , L"Версия"),
};

const std::vector<AddInBase::Alias> ClipboardControl::m_MethList{
	Alias(eEmpty    , 0, false , L"Очистить"   , L"Empty"),
	Alias(eSetData  , 1, true  , L"Записать"   , L"SetData"),
};

/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool ClipboardControl::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum) {
	case eImage:
		return ClipboardManager(this).GetImage(pvarPropVal);
	case eText:
		return VA(pvarPropVal) << ClipboardManager(this).GetText();
	case eFormat:
		return VA(pvarPropVal) << ClipboardManager(this).GetFormat();
	case eVersion:
		return VA(pvarPropVal) << MB2WC(VER_FILE_VERSION_STR);
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool ClipboardControl::SetPropVal(const long lPropNum, tVariant* varPropVal)
{
	switch (lPropNum) {
	case eImage:
		return ClipboardManager(this).SetImage(varPropVal);
	case eText: {
		wchar_t* str = 0;
		::convFromShortWchar(&str, varPropVal->pwstrVal);
		ClipboardManager(this).SetText(str);
		delete[] str;
		return true;
	}
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool ClipboardControl::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum) {
	case eEmpty:
		return ClipboardManager(this).Empty();
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool ClipboardControl::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	return false;
}

