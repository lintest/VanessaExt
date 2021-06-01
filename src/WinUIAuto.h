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
private:
	void find(DWORD pid, IUIAutomationElement** element);
	void find(const std::string& id, IUIAutomationElement** element);
	bool isWindow(IUIAutomationElement* element, JSON& json);
	JSON info(IUIAutomationElement* element, bool subtree = false);
	JSON info(IUIAutomationElementArray* elements);
	UIAutoUniquePtr<IUIAutomation> pAutomation;
	void InitAutomation();
public:
	std::string GetFocusedElement();
	std::string GetElements(DWORD pid);
	std::string FindElements(DWORD pid, const std::wstring& name, const std::string& type, const std::string& parent);
	std::wstring GetElementValue(const std::string& id);
	bool SetElementValue(const std::string& id, const std::wstring& value);
	bool InvokeElement(const std::string& id);
};

#endif//_WINDOWS
