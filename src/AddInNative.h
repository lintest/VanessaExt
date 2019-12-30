#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
// class WindowsControl
class AddInNative : public IComponentBase
{
private:
	enum Props
	{
		eCurrentWindow = 0,
		eActiveWindow,
		eProcessId,
		eProcessList,
		eWindowList,
		eDisplayList,
		eScreenInfo,
		ePropLast      // Always last
	};

	enum Methods
	{
		eGetProcessList = 0,
		eGetProcessInfo,
		eFindProcess,
		eFindTestClient,
		eGetDisplayList,
		eGetDisplayInfo,
		eGetScreenRect,
		eGetWindowList,
		eGetChildWindows,
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
		Alias(long id, long np, bool fn, wchar_t* strEn, wchar_t* strRu):
			id(id),
			np(np),
			fn(fn)
		{
			names[0] = strEn;
			names[1] = strRu;
		}
		const wchar_t* Name(int i) const {
			return names[i].c_str();
		}
	};

	class VarinantHelper {
	private:
		tVariant* pvar = NULL;
		IMemoryManager *mm = NULL;
	public:
		VarinantHelper(const VarinantHelper& va) :pvar(va.pvar), mm(va.mm) {}
		VarinantHelper(tVariant* pvar, IMemoryManager* mm) :pvar(pvar), mm(mm) {}
		VarinantHelper& operator=(const VarinantHelper& va) { pvar = va.pvar; mm = va.mm; return *this; }
		VarinantHelper& operator<<(const wchar_t* str);
		VarinantHelper& operator<<(const std::wstring &str);
		VarinantHelper& operator<<(long int value);
		operator BOOL() const { return true; };
	};

	VarinantHelper VA(tVariant* pvar) { return VarinantHelper(pvar, m_iMemory); } 

	static const std::vector<Alias> m_PropList;
	static const std::vector<Alias> m_MethList;
	long FindName(const std::vector<Alias>& names, const WCHAR_T* name);
	const WCHAR_T* GetName(const std::vector<Alias>& names, long lPropNum, long lPropAlias);
	long GetNParams(const std::vector<Alias>& names, const long lMethodNum);
	bool HasRetVal(const std::vector<Alias>& names, const long lMethodNum);

public:
    AddInNative(void);
    virtual ~AddInNative();
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
    virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant *pvarParamDefValue);   
    virtual bool ADDIN_API HasRetVal(const long lMethodNum);
    virtual bool ADDIN_API CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray);
    virtual bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
    // LocaleBase
    virtual void ADDIN_API SetLocale(const WCHAR_T* loc);
    
private:
	BOOL W(long value, tVariant* res) const;
	const WCHAR_T* W(const wchar_t* str) const;
	BOOL W(const wchar_t* str, WCHAR_T** res) const;
	BOOL W(const wchar_t* str, tVariant* res) const;
	BOOL W(std::string str, tVariant* res) const { return W(MB2WC(str), res); }
	BOOL W(std::wstring str, tVariant* res) const { return W(str.c_str(), res); }
    void addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code);
    // Attributes
    IMemoryManager *m_iMemory;
    IAddInDefBase *m_iConnect;
};

class WcharWrapper
{
public:
#ifdef LINUX_OR_MACOS
    WcharWrapper(const WCHAR_T* str);
#endif
    WcharWrapper(const wchar_t* str);
    ~WcharWrapper();
#ifdef LINUX_OR_MACOS
    operator const WCHAR_T*(){ return m_str_WCHAR; }
    operator WCHAR_T*(){ return m_str_WCHAR; }
#endif
    operator const wchar_t*(){ return m_str_wchar; }
    operator wchar_t*(){ return m_str_wchar; }
private:
    WcharWrapper& operator = (const WcharWrapper& other) { return *this; }
    WcharWrapper(const WcharWrapper& other) { }
private:
#ifdef LINUX_OR_MACOS
    WCHAR_T* m_str_WCHAR;
#endif
    wchar_t* m_str_wchar;
};
#endif //__ADDINNATIVE_H__
