#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"
#include "convertor.h"
#include <string>
#include <vector>

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
		eGetWindowInfo,
		eEnableResizing,
		eTakeScreenshot,
		eCaptureWindow,
		eGetWindowText,
		eSetWindowText,
		eGetWindowState,
		eSetWindowState,
		eMinimizeWindow,
		eRestoreWindow,
		eMaximizeWindow,
		eActivateWindow,
		eTypeInfo,
		eMethLast      // Always last
	};

	static const int m_AliasCount = 2;

	class Alias {
	public:
		const long id;
		const long np;
		const bool fn;
	private:
		std::wstring names[m_AliasCount];
	public:
		Alias(long id, long np, bool fn, WCHAR* strEn, WCHAR* strRu):
			id(id),
			np(np),
			fn(fn)
		{
			names[0] = strEn;
			names[1] = strRu;
		}
		const WCHAR* Name(int i) const {
			return names[i].c_str();
		}
	};

	static const std::vector<Alias> m_PropList;
	static const std::vector<Alias> m_MethList;

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
	long FindName(const std::vector<Alias>& names, const WCHAR_T* name);
	const WCHAR_T* GetName(const std::vector<Alias>& names, long lPropNum, long lPropAlias);
	long GetNParams(const std::vector<Alias>& names, const long lMethodNum);
	bool HasRetVal(const std::vector<Alias>& names, const long lMethodNum);
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
