#pragma once

#ifdef _WINDOWS

#include "stdafx.h"
#include <uiautomation.h>

template<class T>
struct UIDefaultDeleter {
	void operator()(T* a) { if (a) a->Release(); }
};

struct SafeArrayDeleter {
	void operator()(SAFEARRAY* sa) { if (sa) SafeArrayDestroy(sa); }
};

template<class T, class D = UIDefaultDeleter<T>>
using UIAutoUniquePtr = std::unique_ptr<T, D>;

class WinUIAuto {
private:
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
	class UICacheRequest {
	private:
		UIAutoUniquePtr<IUIAutomationCacheRequest> cache;
	public:
		UICacheRequest(WinUIAuto& owner);
		operator IUIAutomationCacheRequest* () { return cache.get(); }
		IUIAutomationCacheRequest* operator->() { return cache.get(); }
	};
private:
	HRESULT find(DWORD pid, UICacheRequest& cache, IUIAutomationElement** element);
	HRESULT find(const std::string& id, UICacheRequest& cache, IUIAutomationElement** element);
	bool isWindow(IUIAutomationElement* element, JSON& json);
	JSON info(IUIAutomationElement* element, UICacheRequest& cache, bool subtree = false);
	JSON info(IUIAutomationElementArray* elements, UICacheRequest& cache);
	UIAutoUniquePtr<IUIAutomation> pAutomation;
	HRESULT hInitialize;
public:
	WinUIAuto();
	virtual ~WinUIAuto();
	std::string GetFocusedElement();
	std::string GetElements(DWORD pid);
	std::string GetElements(const std::string &id);
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
};

#endif//_WINDOWS
