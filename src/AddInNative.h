#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#ifdef _WINDOWS
#include <wtypes.h>
#endif //_WINDOWS

#include <map>
#include <string>
#include <vector>
#include <variant>
#include <functional>

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"

class AddInNative;

class DefaultHelper {
private:
	class EmptyValue {};
public:
	std::variant<
		EmptyValue,
		std::u16string,
		int64_t,
		double,
		bool
	> variant;
public:
	DefaultHelper() : variant(EmptyValue()) {}
	DefaultHelper(const std::u16string& s) : variant(s) {}
	DefaultHelper(int64_t value) : variant(value) {}
	DefaultHelper(double value) : variant(value) {}
	DefaultHelper(bool value) : variant(value) {}
	DefaultHelper(const char16_t* value) {
		if (value) variant = std::u16string(value);
		else variant = EmptyValue();
	}
};

using CompFunction = std::function<AddInNative* ()>;

class AddInNative : public IComponentBase
{
protected:
	class VarinantHelper {
	private:
		tVariant* pvar = nullptr;
		AddInNative* addin = nullptr;
	public:
		void AllocMemory(unsigned long size);
		VarinantHelper(const VarinantHelper& va) :pvar(va.pvar), addin(va.addin) {}
		VarinantHelper(tVariant* pvar, AddInNative* addin) :pvar(pvar), addin(addin) {}
		VarinantHelper& operator<<(const VarinantHelper& va) { pvar = va.pvar; addin = va.addin; return *this; }
		VarinantHelper& operator=(const VarinantHelper& va) = delete;
		VarinantHelper& operator=(const std::string& str);
		VarinantHelper& operator=(const std::wstring& str);
		VarinantHelper& operator=(const std::u16string& str);
		VarinantHelper& operator=(int64_t value);
		VarinantHelper& operator=(double value);
		VarinantHelper& operator=(bool value);
		operator std::string() const;
		operator std::wstring() const;
		operator std::u16string() const;
		operator int64_t() const;
		operator double() const;
		operator bool() const;
		uint32_t size();
		TYPEVAR type();
		char* data();
		void clear();
	};

	using VH = VarinantHelper;
	using MethDefaults = std::map<long, DefaultHelper>;
	using PropFunction = std::function<void(VH)>;
	using MethFunction0 = std::function<void()>;
	using MethFunction1 = std::function<void(VH)>;
	using MethFunction2 = std::function<void(VH, VH)>;
	using MethFunction3 = std::function<void(VH, VH, VH)>;
	using MethFunction4 = std::function<void(VH, VH, VH, VH)>;
	using MethFunction5 = std::function<void(VH, VH, VH, VH, VH)>;
	using MethFunction6 = std::function<void(VH, VH, VH, VH, VH, VH)>;
	using MethFunction7 = std::function<void(VH, VH, VH, VH, VH, VH, VH)>;

	using MethFunction = std::variant<
		MethFunction0,
		MethFunction1,
		MethFunction2,
		MethFunction3,
		MethFunction4,
		MethFunction5,
		MethFunction6,
		MethFunction7
	>;

	void AddProperty(const std::u16string& nameEn, const std::u16string& nameRu, PropFunction getter, PropFunction setter = nullptr);
	void AddProcedure(const std::u16string& nameEn, const std::u16string& nameRu, MethFunction handler, MethDefaults defs = {});
	void AddFunction(const std::u16string& nameEn, const std::u16string& nameRu, MethFunction handler, MethDefaults defs = {});
	static std::u16string AddComponent(const std::u16string& name, CompFunction creator);
	VarinantHelper result;

	static std::u16string AddInNative::upper(std::u16string& str);
	static std::wstring AddInNative::upper(std::wstring& str);
	static std::string WCHAR2MB(std::basic_string_view<WCHAR_T> src);
	static std::wstring WCHAR2WC(std::basic_string_view<WCHAR_T> src);
	static std::u16string MB2WCHAR(std::string_view src);
	WCHAR_T* W(const char16_t* str) const;

private:
	struct Prop {
		std::vector<std::u16string> names;
		PropFunction getter;
		PropFunction setter;
	};

	struct Meth {
		std::vector<std::u16string> names;
		MethFunction handler;
		MethDefaults defs;
		bool hasRetVal;
	};

	bool CallMethod(MethFunction* function, tVariant* paParams, const long lSizeArray);
	VarinantHelper VA(tVariant* pvar) { return VarinantHelper(pvar, this); }
	bool ADDIN_API AllocMemory(void** pMemory, unsigned long ulCountByte) const;
	void ADDIN_API FreeMemory(void** pMemory) const;

	friend const WCHAR_T* GetClassNames();
	static std::u16string AddInNative::getComponentNames();
	friend long GetClassObject(const WCHAR_T*, IComponentBase**);
	static AddInNative* CreateObject(const std::u16string& name);

	static std::map<std::u16string, CompFunction> components;
	std::vector<Prop> properties;
	std::vector<Meth> methods;
	std::u16string name;

public:
	AddInNative(void) : result(nullptr, this) {}
	virtual ~AddInNative() {}
	// IInitDoneBase
	virtual bool ADDIN_API Init(void*) override final;
	virtual bool ADDIN_API setMemManager(void* mem) override final;
	virtual long ADDIN_API GetInfo() override final;
	virtual void ADDIN_API Done() override final;
	// ILanguageExtenderBase
	virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T** wsLanguageExt) override final;
	virtual long ADDIN_API GetNProps() override final;
	virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName) override final;
	virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias) override final;
	virtual bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal) override final;
	virtual bool ADDIN_API SetPropVal(const long lPropNum, tVariant* pvarPropVal) override final;
	virtual bool ADDIN_API IsPropReadable(const long lPropNum) override final;
	virtual bool ADDIN_API IsPropWritable(const long lPropNum) override final;
	virtual long ADDIN_API GetNMethods() override final;
	virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName) override final;
	virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, const long lMethodAlias) override final;
	virtual long ADDIN_API GetNParams(const long lMethodNum) override final;
	virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue) override final;
	virtual bool ADDIN_API HasRetVal(const long lMethodNum) override final;
	virtual bool ADDIN_API CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray) override final;
	virtual bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) override final;
	operator IComponentBase* () { return (IComponentBase*)this; };
	// LocaleBase
	virtual void ADDIN_API SetLocale(const WCHAR_T* loc) override final;
private:
	IMemoryManager* m_iMemory = nullptr;
	IAddInDefBase* m_iConnect = nullptr;
};
#endif //__ADDINNATIVE_H__
