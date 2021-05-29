#pragma once

#ifdef _WINDOWS

#include "stdafx.h"
#include <uiautomation.h>

template<typename T>
struct UIAutoDeleter {
	void operator()(T* a) { if (a) a->Release(); }
};

template<typename T>
using UIAutoUniquePtr = std::unique_ptr<T, UIAutoDeleter<T>>;

class WinUIAuto {
private:
	template<typename T>
	class UI {
	private:
		T* ptr = nullptr;
		UIAutoUniquePtr<T>& src;
	public:
		UI(UIAutoUniquePtr<T>& p) : ptr(p.get()), src(p) { }
		virtual ~UI() { src.reset(ptr); }
		T** operator&() { return &ptr; }
	};
private:
	bool isWindow(IUIAutomationElement* element, JSON& json);
	JSON info(IUIAutomationElement* element, bool subtree);
	UIAutoUniquePtr<IUIAutomation> pAutomation;
	void InitAutomation();
public:
	std::string GetElements(DWORD pid);
	std::string FindElements(DWORD pid, const std::wstring& name, const std::string& type);
};

#endif//_WINDOWS
