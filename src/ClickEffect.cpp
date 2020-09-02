#ifdef _WINDOWS

#include "stdafx.h"
#include "windows.h"
#include "ClickEffect.h"

class ClickEffect::ClickData {
public:
	bool last = false;
	COLORREF color = RGB(200, 50, 50);
	int radius = 30;
	int width = 12;
	int delay = 12;
	int trans = 127;
	int step = 0;
	int max = 0;
public:
	static ClickData settings;
	ClickData() {}
	ClickData(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans)
		: color((COLORREF)color), radius((int)radius), width((int)width), delay((int)delay), trans((int)trans){}
	ClickData(const ClickData& d)
		: color((COLORREF)d.color), radius((int)d.radius), width((int)d.width), delay((int)d.delay), trans((int)d.trans) {}
	ClickData& operator=(const ClickData& d) {
		color = d.color;
		radius = d.radius;
		width = d.width;
		delay = d.delay;
		trans = d.trans;
		return *this;
	}
	HPEN CreatePen();
};

#define ID_CLICK_TIMER 1

HPEN ClickEffect::ClickData::CreatePen()
{
	int w = width * (max - step) / max;
	return ::CreatePen(PS_SOLID, w, color);
}

LRESULT CALLBACK PaintWindow(HWND hWnd)
{
	auto data = (ClickEffect::ClickData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	HPEN pen = data->CreatePen();
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	HPEN hOldPen = (HPEN)SelectObject(hdc, pen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(BLACK_BRUSH));
	int x = data->radius - data->step;
	int d = data->radius + data->step;
	Ellipse(hdc, x, x, d, d);
	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	EndPaint(hWnd, &ps);
	DeleteObject(pen);
	data->step++;
	if (data->step > data->max) {
		if (data->last) {
			SendMessage(hWnd, WM_DESTROY, 0, 0);
			KillTimer(hWnd, ID_CLICK_TIMER);
		}
		else {
			KillTimer(hWnd, ID_CLICK_TIMER);
			SetTimer(hWnd, ID_CLICK_TIMER, data->delay * 2, NULL);
			data->max = data->radius / 2;
			data->last = true;
			data->step = 0;
		}
	}
	return 0L;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_NCCREATE: {
		LPCREATESTRUCT lpcp = (LPCREATESTRUCT)lParam;
		lpcp->style &= (~WS_CAPTION);
		lpcp->style &= (~WS_BORDER);
		SetWindowLong(hWnd, GWL_STYLE, lpcp->style);
		return true;
	}
	case WM_TIMER:
		if (wParam == ID_CLICK_TIMER) {
			InvalidateRect(hWnd, NULL, TRUE);
		}
		return 0;
	case WM_PAINT:
		return PaintWindow(hWnd);
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	auto data = (ClickEffect::ClickData*)lpParam;
	data->max = data->radius;

	POINT p;
	::GetCursorPos(&p);
	int r = data->radius;
	int x = p.x - r - 1;
	int y = p.y - r - 1;
	int w = r * 2 + 2;
	int h = r * 2 + 1;

	LPCWSTR name = L"VanessaClickEffect";
	WNDCLASS wndClass;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hModule;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = name;
	RegisterClass(&wndClass);

	DWORD dwStyle = WS_OVERLAPPED;
	DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
	HWND hWnd = CreateWindowEx(dwExStyle, name, name, dwStyle, x, y, w, h, 0, 0, hModule, 0);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpParam);

	MSG msg;
	SetTimer(hWnd, ID_CLICK_TIMER, data->delay, NULL);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);
	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), data->trans, LWA_COLORKEY | LWA_ALPHA);
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	delete data;
	return 0;
}

ClickEffect::ClickData ClickEffect::ClickData::settings;
HHOOK hMouseHook = NULL;

LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		switch (wParam) {
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN: 
			ClickEffect::Show();
		}
	}
	return CallNextHookEx(hMouseHook, code, wParam, lParam);
}

void ClickEffect::Show()
{
	ClickData* data = new ClickData(ClickData::settings);
	CreateThread(0, NULL, ThreadProc, (LPVOID)data, NULL, NULL);
}

void ClickEffect::Show(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans)
{
	ClickData* data = new ClickData(color, radius, width, delay, trans);
	CreateThread(0, NULL, ThreadProc, (LPVOID)data, NULL, NULL);
}

void ClickEffect::Hook(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans)
{
	if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
	ClickData::settings = ClickData(color, radius, width, delay, trans);
	hMouseHook = SetWindowsHookEx(WH_MOUSE, &HookProc, hModule, NULL);
}

void ClickEffect::Unhook()
{
	if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
	hMouseHook = NULL;
}


#endif //_WINDOWS