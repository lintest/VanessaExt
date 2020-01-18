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
	Alias(eFiles   , true  , L"Files"   , L"СписокФайлов"),
	Alias(eFormat  , false , L"Format"  , L"Формат"),
	Alias(eVersion , false , L"Version" , L"Версия"),
};

const std::vector<AddInBase::Alias> ClipboardControl::m_MethList{
	Alias(eEmpty    , 0, false , L"Очистить"         , L"Empty"),
	Alias(eSetData  , 2, false , L"ЗаписатьДанные"   , L"SetData"),
	Alias(eSetImage , 2, false , L"ЗаписатьКартинку" , L"SetImage"),
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

//---------------------------------------------------------------------------//
bool ClipboardControl::SetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum) {
	case eImage:
		return ClipboardManager(this).SetImage(pvarPropVal);
	case eText: {
		return ClipboardManager(this).SetText(pvarPropVal);
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
	case eSetImage:
		return ClipboardManager(this).SetImage(paParams, lSizeArray);
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool ClipboardControl::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	return false;
}

