#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"
#include "convertor.h"
#include <string>

#define ALIAS_COUNT 2

///////////////////////////////////////////////////////////////////////////////
// class CAddInNative
class CAddInNative : public IComponentBase
{
private:
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
		eFindTestClient,
		eGetWindowList,
		eSetWindowSize,
		eSetWindowPos,
		eEnableResizing,
		eTakeScreenshot,
		eCaptureWindow,
		eGetWindowText,
		eSetWindowText,
		eActivateWindow,
		eMaximizeWindow,
		eRestoreWindow,
		eMinimizeWindow,
		eMethLast      // Always last
	};

	class Alias {
	public:
		const int id;
	private:
		std::wstring names[ALIAS_COUNT];
	public:
		Alias(int id, WCHAR* strEn, WCHAR* strRu): id(id) {
			names[0] = strEn;
			names[1] = strRu;
		}
		const WCHAR_T* Name(int i) const {
			return names[i].c_str();
		}
	};

	static const Alias m_PropNames[];
	static const Alias m_MethNames[];
	static int const m_PropCount;
	static int const m_MethCount;

public:
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
	long FindName(const Alias names[], long size, const WCHAR_T* name);
	const WCHAR_T* GetName(const Alias names[], long size, long lPropNum, long lPropAlias);
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
