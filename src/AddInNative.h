#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"
#include "convertor.h"
#include <string>

#define ALIAS_COUNT 2

typedef wchar_t* ADDIN_NAMES[][ALIAS_COUNT];

static const ADDIN_NAMES g_PropNames = {
	{ L"CurrentWindow", L"ТекущееОкно" },
	{ L"ActiveWindow", L"АктивноеОкно" },
	{ L"ProcessId", L"ИдентификаторПроцесса" },
};

static const ADDIN_NAMES g_MethodNames = {
	{ L"GetProcessList", L"ПолучитьСписокПроцессов" },
	{ L"GetProcessInfo", L"ПолучитьДанныеПроцесса" },
	{ L"FindProcess", L"НайтиПроцесс" },
	{ L"GetWindowList", L"ПолучитьСписокОкон" },
	{ L"SetWindowSize", L"УстановитьРазмерОкна" },
	{ L"SetWindowPos", L"УстановитьПозициюОкна" },
	{ L"EnableResizing", L"РазрешитьИзменятьРазмер" },
	{ L"TakeScreenshot", L"ПолучитьСнимокЭкрана" },
	{ L"CaptureWindow", L"ПолучитьСнимокОкна" },
	{ L"GetWindowText", L"ПолучитьЗаголовок" },
	{ L"SetWindowText", L"УстановитьЗаголовок" },
	{ L"Maximize", L"Максимизировать" },
	{ L"Activate", L"Активировать" },
};

///////////////////////////////////////////////////////////////////////////////
// class CAddInNative
class CAddInNative : public IComponentBase
{
public:
	enum Props
	{
		eCurrentWindow = 0,
		eActiveWindow,
		eProcessId,
		ePropLast      // Always last
	};

	enum Methods
	{
		eGetProcessList = 0,
		eGetProcessInfo,
		eFindProcess,
		eGetWindowList,
		eSetWindowSize,
		eSetWindowPos,
		eEnableResizing,
		eTakeScreenshot,
		eCaptureWindow,
		eGetWindowText,
		eSetWindowText,
		eMaximize,
		eActivate,
		eMethLast      // Always last
	};

	CAddInNative(void);
	virtual ~CAddInNative();
	// IInitDoneBase
	virtual bool ADDIN_API Init(void*);
	virtual bool ADDIN_API setMemManager(void* mem);
	virtual long ADDIN_API GetInfo();
	virtual void ADDIN_API Done();
	// ILanguageExtenderBase
	virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T**);
	virtual long ADDIN_API GetNProps();
	virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName);
	virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias);
	virtual bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal);
	virtual bool ADDIN_API SetPropVal(const long lPropNum, tVariant* varPropVal);
	virtual bool ADDIN_API IsPropReadable(const long lPropNum);
	virtual bool ADDIN_API IsPropWritable(const long lPropNum);
	virtual long ADDIN_API GetNMethods();
	virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName);
	virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, const long lMethodAlias);
	virtual long ADDIN_API GetNParams(const long lMethodNum);
	virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue);
	virtual bool ADDIN_API HasRetVal(const long lMethodNum);
	virtual bool ADDIN_API CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray);
	virtual bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
	// LocaleBase
	virtual void ADDIN_API SetLocale(const WCHAR_T* loc);

private:
	long FindName(const ADDIN_NAMES names, long size, const WCHAR_T* name);
	const WCHAR_T* GetName(const ADDIN_NAMES names, long size, long lPropNum, long lPropAlias);
	void addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code);
	BOOL W(std::wstring str, tVariant* res) const { return W(str.c_str(), res); }
	BOOL W(std::string str, tVariant* res) const { return W(MB2WC(str), res); }
	BOOL W(const wchar_t* str, WCHAR_T** res) const;
	BOOL W(const wchar_t* str, tVariant* res) const;
	BOOL W(const DWORD& val, tVariant* res) const;
	const WCHAR_T* W(const wchar_t* str) const;
	// Attributes
	IAddInDefBase* m_iConnect;
	IMemoryManager* m_iMemory;
};

#endif //__ADDINNATIVE_H__
