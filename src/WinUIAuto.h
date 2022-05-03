#pragma once

#ifdef _WINDOWS

#include "stdafx.h"
#include <uiautomation.h>

class AddInNative;

template<class T>
struct UIDefaultDeleter {
	void operator()(T* a) { if (a) a->Release(); }
};

struct SafeArrayDeleter {
	void operator()(SAFEARRAY* sa) { if (sa) SafeArrayDestroy(sa); }
};

template<class T, class D = UIDefaultDeleter<T>>
using UIAutoUniquePtr = std::unique_ptr<T, D>;

template<class T, class D = UIDefaultDeleter<T>>
class UI {
private:
	T* ptr = nullptr;
	UIAutoUniquePtr<T, D>& src;
public:
	UI(UIAutoUniquePtr<T, D>& p) : ptr(p.get()), src(p) { }
	virtual ~UI() { src.reset(ptr); }
	T** operator&() { return &ptr; }
	operator T** () { return &ptr; }
	operator T* () { return ptr; }
};

class WinUIAuto;

class UICacheRequest {
private:
	UIAutoUniquePtr<IUIAutomationCacheRequest> cache;
public:
	UICacheRequest(WinUIAuto& owner);
	operator IUIAutomationCacheRequest* () { return cache.get(); }
	IUIAutomationCacheRequest* operator->() { return cache.get(); }
};

#define UI_AUTO_HANDLER(IID_IHandler)                                                         \
private:                                                                                      \
	ULONG volatile m_count = 1;                                                               \
	WinUIAuto& m_owner;                                                                       \
	UICacheRequest m_cache;                                                                   \
	AddInNative* m_addin = nullptr;                                                           \
public:                                                                                       \
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj) override {  \
		if (!ppvObj) {                                                                        \
			return E_INVALIDARG;                                                              \
		}                                                                                     \
		*ppvObj = NULL;                                                                       \
		if (riid == IID_IUnknown || riid == IID_IHandler) {                                   \
			AddRef();                                                                         \
			return NOERROR;                                                                   \
		}                                                                                     \
		return E_NOINTERFACE;                                                                 \
	}                                                                                         \
	virtual ULONG STDMETHODCALLTYPE AddRef() override {                                       \
		InterlockedIncrement(&m_count);                                                       \
		return m_count;                                                                       \
	}                                                                                         \
	virtual ULONG STDMETHODCALLTYPE Release() override {                                      \
		ULONG ulRefCount = InterlockedDecrement(&m_count);                                    \
		if (0 == m_count) {                                                                   \
			delete this;                                                                      \
		}                                                                                     \
		return ulRefCount;                                                                    \
	}

class UIAutoFocusHandler
	: public IUIAutomationFocusChangedEventHandler
{
	UI_AUTO_HANDLER(IID_IUIAutomationFocusChangedEventHandler)
private:
	UIAutoFocusHandler(WinUIAuto& owner, AddInNative* addin);
public:
	static UIAutoFocusHandler* CreateInstance(WinUIAuto& owner, AddInNative* addin);
	void ResetHandler();
public:
	virtual HRESULT STDMETHODCALLTYPE HandleFocusChangedEvent(IUIAutomationElement* sender) override;
};

class UIAutoEventHandler
	: public IUIAutomationEventHandler
{
	UI_AUTO_HANDLER(IID_IUIAutomationFocusChangedEventHandler)
private:
	IUIAutomationElement* m_sender = nullptr;
	UIAutoEventHandler(WinUIAuto& owner, AddInNative* addin, IUIAutomationElement* element);
public:
	static UIAutoEventHandler* CreateInstance(WinUIAuto& owner, AddInNative* addin);
	void ResetHandler();
public:
	virtual HRESULT STDMETHODCALLTYPE HandleAutomationEvent(IUIAutomationElement* sender, EVENTID eventId) override;
};

template<class T>
struct UIHandlerDeleter {
	void operator()(T* a) {
		if (a) {
			a->ResetHandler();
			a->Release();
		}
	}
};

class WinUIAuto {
private:
	bool isWindow(IUIAutomationElement* element, JSON& json);
	HRESULT find(DWORD pid, UICacheRequest& cache, IUIAutomationElement** element);
	HRESULT find(const std::string& id, UICacheRequest& cache, IUIAutomationElement** element);
	std::unique_ptr<UIAutoEventHandler, UIHandlerDeleter<UIAutoEventHandler>> m_focusHandler;
	UIAutoUniquePtr<IUIAutomation> m_automation;
	HRESULT hInitialize;
public:
	JSON info(IUIAutomationElement* element, UICacheRequest& cache, int64_t level = -1);
	JSON info(IUIAutomationElementArray* elements, UICacheRequest& cache);
	IUIAutomation* getAutomation() { return m_automation.get(); }
public:
	WinUIAuto();
	virtual ~WinUIAuto();
	std::string GetFocusedElement();
	std::string GetElements(DWORD pid, int64_t level);
	std::string GetElements(const std::string &id, int64_t level);
	std::string ElementById(const std::string& id);
	std::string ElementFromPoint(int x, int y);
	std::string FindElements(const std::string& arg);
	std::string GetParentElement(const std::string& id);
	std::string GetNextElement(const std::string& id);
	std::string GetPreviousElement(const std::string& id);
	std::wstring GetElementValue(const std::string& id);
	bool SetElementValue(const std::string& id, const std::wstring& value);
	bool InvokeElement(const std::string& id);
	bool FocusElement(const std::string& id);
	void setMonitoringStatus(AddInNative* addin);
	bool getMonitoringStatus();
};

#endif//_WINDOWS
