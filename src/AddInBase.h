#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
// class WindowsControl
class AddInBase : public IComponentBase
{
protected:
	static const int m_AliasCount = 2;

	class Alias {
	public:
		const long id;
		const long np;
		const bool fn;
	private:
		std::wstring names[m_AliasCount];
	public:
		Alias(long id, long np, bool fn, wchar_t* strEn, wchar_t* strRu)
			:id(id), np(np), fn(fn)
		{
			names[0] = strEn;
			names[1] = strRu;
		}
		Alias(long id, bool fn, wchar_t* strEn, wchar_t* strRu)
			:id(id), np(0), fn(fn)
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
		IMemoryManager* mm = NULL;
	public:
		VarinantHelper(const VarinantHelper& va) :pvar(va.pvar), mm(va.mm) {}
		VarinantHelper(tVariant* pvar, IMemoryManager* mm) :pvar(pvar), mm(mm) {}
		VarinantHelper& operator=(const VarinantHelper& va) { pvar = va.pvar; mm = va.mm; return *this; }
		VarinantHelper& operator<<(const wchar_t* str);
		VarinantHelper& operator<<(const std::wstring& str);
		VarinantHelper& operator<<(int32_t value);
		VarinantHelper& operator<<(int64_t value);
		operator BOOL() const { return true; };
	};

	VarinantHelper VA(tVariant* pvar) { return VarinantHelper(pvar, m_iMemory); }

	virtual const std::vector<Alias>& PropList() const = 0;
	virtual const std::vector<Alias>& MethList() const = 0;
	virtual const wchar_t* ExtensionName() const = 0;

	long FindName(const std::vector<Alias>& names, const WCHAR_T* name);
	const WCHAR_T* GetName(const std::vector<Alias>& names, long lPropNum, long lPropAlias);
	long GetNParams(const std::vector<Alias>& names, const long lMethodNum);
	bool HasRetVal(const std::vector<Alias>& names, const long lMethodNum);

public:
	AddInBase(void);
	virtual ~AddInBase();
	// IInitDoneBase
	bool ADDIN_API Init(void*) override;
	bool ADDIN_API setMemManager(void* mem) override;
	long ADDIN_API GetInfo() override;
	void ADDIN_API Done() override;
	// ILanguageExtenderBase
	bool ADDIN_API RegisterExtensionAs(WCHAR_T**) override;
	long ADDIN_API FindProp(const WCHAR_T* wsPropName) override;
	long ADDIN_API GetNProps() override;
	const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias) override;
	bool ADDIN_API IsPropReadable(const long lPropNum) override;
	bool ADDIN_API IsPropWritable(const long lPropNum) override;
	long ADDIN_API GetNMethods() override;
	long ADDIN_API FindMethod(const WCHAR_T* wsMethodName) override;
	const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, const long lMethodAlias) override;
	long ADDIN_API GetNParams(const long lMethodNum) override;
	bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue) override;
	bool ADDIN_API HasRetVal(const long lMethodNum) override;
	// LocaleBase
	void ADDIN_API SetLocale(const WCHAR_T* loc) override;

protected:
	const WCHAR_T* W(const wchar_t* str) const;
	void addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code);
	IMemoryManager* m_iMemory;
	IAddInDefBase* m_iConnect;
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
	operator const WCHAR_T* () { return m_str_WCHAR; }
	operator WCHAR_T* () { return m_str_WCHAR; }
#endif
	operator const wchar_t* () { return m_str_wchar; }
	operator wchar_t* () { return m_str_wchar; }
private:
	WcharWrapper& operator = (const WcharWrapper& other) { return *this; }
	WcharWrapper(const WcharWrapper& other) { }
private:
#ifdef LINUX_OR_MACOS
	WCHAR_T* m_str_WCHAR;
#endif
	wchar_t* m_str_wchar;
};

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);

#endif //__ADDINNATIVE_H__
