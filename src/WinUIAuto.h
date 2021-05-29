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
	JSON info(IUIAutomationElement* element);
	UIAutoUniquePtr<IUIAutomation> pAutomation;
public:
	std::string GetElements(int64_t pid);
};

#endif//_WINDOWS
