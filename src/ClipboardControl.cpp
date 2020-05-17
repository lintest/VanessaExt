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
	Alias(eFiles   , true  , L"Files"   , L"Файлы"),
	Alias(eImage   , true  , L"Image"   , L"Картинка"),
	Alias(eFormat  , false , L"Format"  , L"Формат"),
	Alias(eVersion , false , L"Version" , L"Версия"),
};

const std::vector<AddInBase::Alias> ClipboardControl::m_MethList{
	Alias(eEmpty    , 0, false , L"Очистить"         , L"Empty"),
	Alias(eSetText  , 1, false , L"ЗаписатьТекст"    , L"SetText"),
	Alias(eSetData  , 1, false , L"ЗаписатьДанные"   , L"SetData"),
	Alias(eSetImage , 1, false , L"ЗаписатьКартинку" , L"SetImage"),
	Alias(eSetFiles , 1, false , L"ЗаписатьФайлы"   ,  L"SetFiles"),
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
	case eFiles:
		return VA(pvarPropVal) << ClipboardManager(this).GetFiles();
	case eFormat:
		return VA(pvarPropVal) << ClipboardManager(this).GetFormat();
	case eVersion:
		return VA(pvarPropVal) << MB2WC(VER_FILE_VERSION_STR);
	default:
		return false;
	}
}

#define ASSERT(c, m) if (!(c)) { addError(m); return false; }

//---------------------------------------------------------------------------//
bool ClipboardControl::SetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum) {
	case eImage:
		ASSERT(TV_VT(pvarPropVal) == VTYPE_BLOB, L"Parameter type mismatch.");
		return ClipboardManager(this).SetImage(pvarPropVal);
	case eText: {
		ASSERT(TV_VT(pvarPropVal) == VTYPE_PWSTR, L"Parameter type mismatch.");
		return ClipboardManager(this).SetText(pvarPropVal);
	case eFiles:
		ASSERT(TV_VT(pvarPropVal) == VTYPE_PWSTR, L"Parameter type mismatch.");
		return ClipboardManager(this).SetFiles(pvarPropVal);
	}
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool ClipboardControl::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{
	return false;
}
//---------------------------------------------------------------------------//
bool ClipboardControl::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum) {
	case eEmpty:
		return VA(pvarRetValue) << ClipboardManager(this).Empty();
	case eSetText:
		ASSERT(TV_VT(paParams) == VTYPE_PWSTR, L"Parameter type mismatch.");
		return VA(pvarRetValue) << ClipboardManager(this).SetText(paParams, false);
	case eSetImage:
		ASSERT(TV_VT(paParams) == VTYPE_BLOB, L"Parameter type mismatch.");
		return VA(pvarRetValue) << ClipboardManager(this).SetImage(paParams, false);
	case eSetFiles:
		ASSERT(TV_VT(paParams) == VTYPE_PWSTR, L"Parameter type mismatch.");
		return VA(pvarRetValue) << ClipboardManager(this).SetFiles(paParams, false);
	default:
		return false;
	}
}
