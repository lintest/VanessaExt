#include "stdafx.h"
#include "ClipboardControl.h"
#include "ClipboardManager.h"

std::vector<std::u16string> ClipboardControl::names = {
	AddComponent(u"ClipboardControl", []() { return new ClipboardControl; }),
};

ClipboardControl::ClipboardControl()
{
	AddProperty(u"Text", u"Текст",
		[&](VH var) { var = ClipboardManager().GetText(); },
		[&](VH var) { ClipboardManager().SetText(var); }
	);
	AddProperty(u"Files", u"Файлы", 
		[&](VH var) { var = ClipboardManager().GetFiles(); },
		[&](VH var) { ClipboardManager().SetFiles(var); }
	);
	AddProperty(u"Image", u"Картинка", 
		[&](VH var) { ClipboardManager().GetImage(var); },
		[&](VH var) { ClipboardManager().SetImage(var); }
	);
	AddProperty(u"Format", u"Формат",
		[&](VH var) { var = ClipboardManager().GetFormat(); }
	);

#ifdef _WINDOWS
	AddProperty(u"Monitoring", u"Мониторинг",
		[&](VH value) { value = this->GetMonitoring(); },
		[&](VH value) { this->SetMonitoring(value); }
	);
#endif //_WINDOWS
	
	AddFunction(u"Empty", u"Очистить", [&]() { this->result = ClipboardManager().Empty(); });
	AddFunction(u"SetText", u"ЗаписатьТекст", [&](VH var) { this->result = ClipboardManager().SetText(var); });
	AddFunction(u"SetFiles", u"ЗаписатьФайлы", [&](VH var) { this->result = ClipboardManager().SetFiles(var); });
	AddFunction(u"SetImage", u"ЗаписатьКартинку", [&](VH var) { this->result = ClipboardManager().SetImage(var); });
}

ClipboardControl::~ClipboardControl()
{
#ifdef _WINDOWS
	SetMonitoring(false);
#endif //_WINDOWS
}

#ifdef _WINDOWS

//////////////////////////////////////////////////////////////////////////////
//                                                                          // 
//  LINUX: https://github.com/cdown/clipnotify/blob/master/clipnotify.c     //
//                                                                          // 
//////////////////////////////////////////////////////////////////////////////

void ClipboardControl::SendEvent()
{
	ClipboardManager manager;
	std::u16string format = MB2WCHAR(manager.GetFormat());
	std::u16string text = MB2WCHAR(WC2MB(manager.GetText()));
	ExternalEvent(format, text);
}

static LRESULT CALLBACK ClipboardWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLIPBOARDUPDATE: {
		auto component = (ClipboardControl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (component) component->SendEvent();
		return 0;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

bool ClipboardControl::GetMonitoring()
{
	return hClipboardMonitor;
}

void ClipboardControl::SetMonitoring(bool value)
{
	if (hClipboardMonitor) {
		SetWindowLongPtr(hClipboardMonitor, GWLP_USERDATA, 0);
		RemoveClipboardFormatListener(hClipboardMonitor);
		DestroyWindow(hClipboardMonitor);
		hClipboardMonitor = nullptr;
	}
	if (value) {
		const LPCWSTR wsClassName = L"VanessaClipboardMonitor";

		WNDCLASS wndClass = {};
		wndClass.hInstance = hModule;
		wndClass.lpszClassName = wsClassName;
		wndClass.lpfnWndProc = ClipboardWndProc;
		RegisterClass(&wndClass);

		hClipboardMonitor = CreateWindow(wsClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hModule, 0);
		SetWindowLongPtr(hClipboardMonitor, GWLP_USERDATA, (LONG_PTR)this);
		AddClipboardFormatListener(hClipboardMonitor);
	}
}

#endif //_WINDOWS
