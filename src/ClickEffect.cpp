#ifdef _WINDOWS

#include "stdafx.h"
#include "windows.h"
#include "ClickEffect.h"

class ClickEffect::Hooker {
public:
	Hooker() {}

	Hooker(
		int64_t color,
		int64_t radius,
		int64_t width,
		int64_t delay,
		int64_t trans
	) :
		color((COLORREF)color),
		radius((int)radius),
		width((int)width),
		delay((int)delay),
		trans((int)trans) {}

	Hooker& operator=(const Hooker& d) {
		color = d.color;
		radius = d.radius;
		width = d.width;
		delay = d.delay;
		trans = d.trans;
		return *this;
	}

	LRESULT Show();
	void Create();
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
	Painter(const Hooker& s) :
		color(s.color),
		radius(s.radius),
		width(s.width),
		delay(s.delay),
		trans(s.trans),
		limit(s.radius) {}
	LRESULT Paint(HWND hWnd);
	void OnTimer(HWND hWnd);
	void Create();
};

#define ID_CLICK_TIMER 1

ClickEffect::Hooker* ClickEffect::hooker(HWND hWnd)
{
	return (ClickEffect::Hooker*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

ClickEffect::Hooker* ClickEffect::hooker(LPVOID lpParam)
{
	return (ClickEffect::Hooker*)lpParam;
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
	return 0L;
}

void ClickEffect::Painter::OnTimer(HWND hWnd)
{
	step++;
	BOOL bErase = false;
	if (step > limit) {
		bErase = true;
		KillTimer(hWnd, ID_CLICK_TIMER);
		if (last) {
			SendMessage(hWnd, WM_DESTROY, 0, 0);
			return;
		}
		else {
			SetTimer(hWnd, ID_CLICK_TIMER, delay * 2, NULL);
			limit = radius / 2;
			last = true;
			step = 0;
		}
	}
	InvalidateRect(hWnd, NULL, bErase);
}

LRESULT CALLBACK EffectWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
			ClickEffect::painter(hWnd)->OnTimer(hWnd);
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

const UINT WM_SHOW_CLICK = WM_USER + 1;

LRESULT CALLBACK HookerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SHOW_CLICK:
		return ClickEffect::hooker(hWnd)->Show();
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
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = EffectWndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hModule;
	wndClass.hIcon = NULL;
	wndClass.hCursor = NULL;
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = name;
	RegisterClass(&wndClass);

	DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;
	HWND hWnd = CreateWindowEx(dwExStyle, name, name, WS_POPUP, x, y, w, h, NULL, NULL, hModule, 0);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

	SetTimer(hWnd, ID_CLICK_TIMER, delay, NULL);
	SetLayeredWindowAttributes(hWnd, 0, trans, LWA_COLORKEY | LWA_ALPHA);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);
}

DWORD WINAPI EffectThreadProc(LPVOID lpParam)
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


DWORD WINAPI HookerThreadProc(LPVOID lpParam)
{
	auto hooker = ClickEffect::hooker(lpParam);
	hooker->Create();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	delete hooker;
	return 0;
}

LRESULT ClickEffect::Hooker::Show()
{
	Painter* painter = new Painter(*this);
	CreateThread(0, NULL, EffectThreadProc, (LPVOID)painter, NULL, NULL);
	return 0L;
}

const LPCWSTR wsHookerName = L"VanessaClickHooker";

static HHOOK hMouseHook = NULL;

LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0) {
		switch (wParam) {
		case WM_RBUTTONUP:
		case WM_LBUTTONUP:
			HWND hWnd = ::FindWindow(wsHookerName, NULL);
			if (hWnd) ::SendMessage(hWnd, WM_SHOW_CLICK, 0, 0);
			break;
		}
	}
	return CallNextHookEx(hMouseHook, code, wParam, lParam);
}

void ClickEffect::Hooker::Create()
{
	WNDCLASS wndClass;
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = HookerWndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hModule;
	wndClass.hIcon = NULL;
	wndClass.hCursor = NULL;
	wndClass.hbrBackground = NULL;
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = wsHookerName;
	RegisterClass(&wndClass);

	HWND hWnd = CreateWindow(wsHookerName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hModule, 0);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

	hMouseHook = SetWindowsHookEx(WH_MOUSE, &HookProc, hModule, NULL);
}

void ClickEffect::Show(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans)
{
	Painter* painter = new Painter(Hooker(color, radius, width, delay, trans));
	CreateThread(0, NULL, EffectThreadProc, (LPVOID)painter, NULL, NULL);
}

typedef void(__cdecl* StartHookProc)(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans);
typedef void(__cdecl* StopHookProc)();

static bool GetLibraryFile(std::wstring& path)
{
	path.resize(MAX_PATH);
	while (true) {
		DWORD res = GetModuleFileName(hModule, path.data(), (DWORD)path.size());
		if (res && res < path.size()) return true;
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return false;
		path.resize(path.size() * 2);
	}
}

static HMODULE LoadHookLibrary()
{
	std::wstring path;
	return GetLibraryFile(path) ? LoadLibrary(path.c_str()) : nullptr;
}

void ClickEffect::Hook(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans)
{
	if (auto h = LoadHookLibrary()) {
		auto proc = (StartHookProc)GetProcAddress(h, "StartClickHook");
		if (proc) proc(color, radius, width, delay, trans);
	}
}

void ClickEffect::Unhook()
{
	if (auto h = LoadHookLibrary()) {
		auto proc = (StopHookProc)GetProcAddress(h, "StopClickHook");
		if (proc) proc();
	}
}

void ClickEffect::Show()
{
	HWND hWnd = ::FindWindow(wsHookerName, NULL);
	if (hWnd) ::SendMessage(hWnd, WM_SHOW_CLICK, 0, 0);
}

extern "C" {
	__declspec(dllexport) void __cdecl StopClickHook()
	{
		HWND hWnd = FindWindow(wsHookerName, NULL);
		if (hWnd) PostMessage(hWnd, WM_DESTROY, 0, 0);
		if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
	}
	__declspec(dllexport) void __cdecl StartClickHook(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans)
	{
		StopClickHook();
		ClickEffect::Hooker* settings = new ClickEffect::Hooker(color, radius, width, delay, trans);
		CreateThread(0, NULL, HookerThreadProc, (LPVOID)settings, NULL, NULL);
	}
}

#endif //_WINDOWS