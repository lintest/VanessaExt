#ifdef _WINDOWS

#include "stdafx.h"
#include "windows.h"
#include "ClickEffect.h"

class ClickEffect::Settings {
public:
	static Settings settings;

	Settings() {}

	~Settings();

	Settings(
		int64_t color, 
		int64_t radius, 
		int64_t width, 
		int64_t delay, 
		int64_t trans
	): 
		color((COLORREF)color), 
		radius((int)radius), 
		width((int)width), 
		delay((int)delay), 
		trans((int)trans) {}

	Settings& operator=(const Settings& d) {
		color = d.color;
		radius = d.radius;
		width = d.width;
		delay = d.delay;
		trans = d.trans;
		return *this;
	}
private:
	COLORREF color = RGB(200, 50, 50);
	int radius = 30;
	int width = 12;
	int delay = 12;
	int trans = 127;
	friend Painter;
};

class ClickEffect::Painter {
private:
	const COLORREF color;
	const int radius;
	const int width;
	const int delay;
	const int trans;
	bool last = false;
	int limit = 0;
	int step = 0;
public:
	Painter(const Settings& s) :
		color(s.color),
		radius(s.radius),
		width(s.width),
		delay(s.delay),
		trans(s.trans),
		limit(s.radius) {}
	LRESULT Paint(HWND hWnd);
	void Create();
};

#define ID_CLICK_TIMER 1

HHOOK ClickEffect::hMouseHook = NULL;

ClickEffect::Settings ClickEffect::Settings::settings;

ClickEffect::Settings::~Settings()
{
	ClickEffect::Unhook();
}

ClickEffect::Painter* ClickEffect::painter(HWND hWnd)
{
	return (ClickEffect::Painter*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

ClickEffect::Painter* ClickEffect::painter(LPVOID lpParam)
{
	return (ClickEffect::Painter*)lpParam;
}

LRESULT ClickEffect::Painter::Paint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	int w = width * (limit - step) / limit;
	HPEN pen = CreatePen(PS_SOLID, w, color);
	HPEN hOldPen = (HPEN)SelectObject(hdc, pen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(BLACK_BRUSH));
	int x = radius - step;
	int d = radius + step;
	Ellipse(hdc, x, x, d, d);
	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	EndPaint(hWnd, &ps);
	DeleteObject(pen);
	step++;
	if (step > limit) {
		KillTimer(hWnd, ID_CLICK_TIMER);
		if (last) {
			SendMessage(hWnd, WM_DESTROY, 0, 0);
		}
		else {
			SetTimer(hWnd, ID_CLICK_TIMER, delay * 2, NULL);
			limit = radius / 2;
			last = true;
			step = 0;
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
		return ClickEffect::painter(hWnd)->Paint(hWnd);
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

void ClickEffect::Painter::Create()
{
	POINT p;
	::GetCursorPos(&p);
	int r = radius;
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
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

	SetTimer(hWnd, ID_CLICK_TIMER, delay, NULL);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);
	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), trans, LWA_COLORKEY | LWA_ALPHA);
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	auto painter = ClickEffect::painter(lpParam);
	painter->Create();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	delete painter;
	return 0;
}

LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		switch (wParam) {
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN:
			ClickEffect::Show();
		}
	}
	return CallNextHookEx(ClickEffect::hMouseHook, code, wParam, lParam);
}

void ClickEffect::Show()
{
	Painter* painter = new Painter(Settings::settings);
	CreateThread(0, NULL, ThreadProc, (LPVOID)painter, NULL, NULL);
}

void ClickEffect::Show(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans)
{
	Painter* painter = new Painter(Settings(color, radius, width, delay, trans));
	CreateThread(0, NULL, ThreadProc, (LPVOID)painter, NULL, NULL);
}

void ClickEffect::Hook(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans)
{
	if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
	Settings::settings = Settings(color, radius, width, delay, trans);
	hMouseHook = SetWindowsHookEx(WH_MOUSE, &HookProc, hModule, NULL);
}

void ClickEffect::Unhook()
{
	if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
	hMouseHook = NULL;
}

#endif //_WINDOWS