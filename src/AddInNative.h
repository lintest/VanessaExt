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
private:
	struct Prop;
	struct Meth;
protected:
	class VarinantHelper {
	private:
		tVariant* pvar = nullptr;
		AddInNative* addin = nullptr;
		Prop* prop = nullptr;
		Meth* meth = nullptr;
		long number = -1;
	private:
		std::exception error(TYPEVAR vt) const;
	public:
		void AllocMemory(unsigned long size);
		VarinantHelper(const VarinantHelper& va) :pvar(va.pvar), addin(va.addin), prop(va.prop), meth(va.meth), number(va.number) {}
		VarinantHelper(tVariant* pvar, AddInNative* addin) :pvar(pvar), addin(addin) {}
		VarinantHelper(tVariant* pvar, AddInNative* addin, Prop* prop) :pvar(pvar), addin(addin), prop(prop) {}
		VarinantHelper(tVariant* pvar, AddInNative* addin, Meth* meth, long number) :pvar(pvar), addin(addin), meth(meth), number(number) {}
		VarinantHelper& operator<<(const VarinantHelper& va) { pvar = va.pvar; addin = va.addin; prop = va.prop, meth = va.meth, number = va.number; return *this; }
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
		operator int() const;
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

	void AddProperty(const std::u16string& nameEn, const std::u16string& nameRu, const PropFunction &getter, const PropFunction &setter = nullptr);
	void AddProcedure(const std::u16string& nameEn, const std::u16string& nameRu, const MethFunction &handler, const MethDefaults &defs = {});
	void AddFunction(const std::u16string& nameEn, const std::u16string& nameRu, const MethFunction &handler, const MethDefaults &defs = {});
	static std::u16string AddComponent(const std::u16string& name, CompFunction creator);
	VarinantHelper result;

public:
	WCHAR_T* W(const char16_t* str) const;
	static std::string version();

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

	bool CallMethod(MethFunction* function, tVariant* paParams, Meth* meth, const long lSizeArray);
	VarinantHelper VA(tVariant* pvar) { return VarinantHelper(pvar, this); }
	VarinantHelper VA(tVariant* pvar, Prop* prop) { return VarinantHelper(pvar, this, prop); }
	VarinantHelper VA(tVariant* pvar, Meth* meth, long number) { return VarinantHelper(pvar + number, this, meth, number); }
	bool ADDIN_API AllocMemory(void** pMemory, unsigned long ulCountByte) const;
	void ADDIN_API FreeMemory(void** pMemory) const;

	friend const WCHAR_T* GetClassNames();
	static std::u16string getComponentNames();
	friend long GetClassObject(const WCHAR_T*, IComponentBase**);
	static AddInNative* CreateObject(const std::u16string& name);

	static std::map<std::u16string, CompFunction> components;
	std::vector<Prop> properties;
	std::vector<Meth> methods;
	std::u16string name;
	bool alias = false;

protected:
	bool ExternalEvent(const std::u16string& message, const std::u16string& data);
	bool AddError(const std::u16string& descr, long scode = 0);
	std::u16string fullname() { return u"AddIn." + name; }

public:
	AddInNative(void) ;
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
protected:
	IAddInDefBase* connection() { return m_iConnect; };
private:
	IMemoryManager* m_iMemory = nullptr;
	IAddInDefBase* m_iConnect = nullptr;
};
#endif //__ADDINNATIVE_H__
