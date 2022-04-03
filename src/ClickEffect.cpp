#ifdef _WINDOWS

#include "stdafx.h"
#include "ClickEffect.h"

namespace ClickEffect {

	class Hooker {
	public:
		Hooker() {}

		Hooker(
			int64_t color,
			int64_t radius,
			int64_t width,
			int64_t delay,
			int64_t trans,
			int64_t echo
		) :
			color((COLORREF)color),
			radius((int)radius),
			width((int)width),
			delay((int)delay),
			trans((int)trans),
			echo((int)echo) {}

		Hooker& operator=(const Hooker& d) {
			color = d.color;
			radius = d.radius;
			width = d.width;
			delay = d.delay;
			trans = d.trans;
			echo = d.echo;
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
		int echo = 1;
		friend Painter;
	};

	class Painter {
	private:
		const COLORREF color;
		const int radius;
		const int width;
		const int delay;
		const int trans;
		const int echo;
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
			echo(s.echo),
			limit(s.radius) {}
		LRESULT Paint(HWND hWnd);
		void OnTimer(HWND hWnd);
		void Create();
	};

#define ID_CLICK_TIMER 1

	Hooker* getHooker(HWND hWnd)
	{
		return (Hooker*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}

	Hooker* getHooker(LPVOID lpParam)
	{
		return (Hooker*)lpParam;
	}

	Painter* getPainter(HWND hWnd)
	{
		return (Painter*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}

	Painter* getPainter(LPVOID lpParam)
	{
		return (Painter*)lpParam;
	}

	LRESULT Painter::Paint(HWND hWnd)
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

	void Painter::OnTimer(HWND hWnd)
	{
		step++;
		BOOL bErase = false;
		if (step > limit) {
			bErase = true;
			KillTimer(hWnd, ID_CLICK_TIMER);
			if (last || echo == 0) {
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
				getPainter(hWnd)->OnTimer(hWnd);
			}
			return 0;
		case WM_PAINT:
			return getPainter(hWnd)->Paint(hWnd);
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
			return getHooker(hWnd)->Show();
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	void Painter::Create()
	{
		POINT p;
		::GetCursorPos(&p);
		int r = radius;
		int x = p.x - r - 1;
		int y = p.y - r - 1;
		int w = r * 2 + 2;
		int h = r * 2 + 1;

		LPCWSTR name = L"VanessaClickEffect";
		WNDCLASS wndClass = {};
		wndClass.style = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc = EffectWndProc;
		wndClass.hInstance = hModule;
		wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wndClass.lpszClassName = name;
		RegisterClass(&wndClass);

		DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;
		HWND hWnd = CreateWindowEx(dwExStyle, name, name, WS_POPUP, x, y, w, h, NULL, NULL, hModule, 0);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
		SetLayeredWindowAttributes(hWnd, 0, trans, LWA_COLORKEY | LWA_ALPHA);
		SetTimer(hWnd, ID_CLICK_TIMER, delay, NULL);
		ShowWindow(hWnd, SW_SHOWNOACTIVATE);
		UpdateWindow(hWnd);
	}

	DWORD WINAPI EffectThreadProc(LPVOID lpParam)
	{
		auto painter = getPainter(lpParam);
		painter->Create();
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		delete painter;
		return 0;
	}


	static DWORD WINAPI HookerThreadProc(LPVOID lpParam)
	{
		auto hooker = getHooker(lpParam);
		hooker->Create();
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		delete hooker;
		return 0;
	}

	LRESULT Hooker::Show()
	{
		auto painter = new Painter(*this);
		CreateThread(0, NULL, EffectThreadProc, (LPVOID)painter, NULL, NULL);
		return 0L;
	}

	const LPCWSTR wsHookerName = L"VanessaClickEffectHooker";

	static HHOOK hMouseHook = NULL;

	LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION) {
			switch (wParam) {
			case WM_RBUTTONUP:
			case WM_LBUTTONUP:
				HWND hWnd = ::FindWindow(wsHookerName, NULL);
				if (hWnd) ::SendMessage(hWnd, WM_SHOW_CLICK, 0, 0);
				break;
			}
		}
		return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
	}

	void Hooker::Create()
	{
		WNDCLASS wndClass = {};
		wndClass.hInstance = hModule;
		wndClass.lpfnWndProc = HookerWndProc;
		wndClass.lpszClassName = wsHookerName;
		RegisterClass(&wndClass);

		HWND hWnd = CreateWindow(wsHookerName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hModule, 0);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

		hMouseHook = SetWindowsHookEx(WH_MOUSE, &HookProc, hModule, NULL);
	}

	void Show(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans, int64_t echo)
	{
		auto painter = new Painter(Hooker(color, radius, width, delay, trans, echo));
		CreateThread(0, NULL, EffectThreadProc, (LPVOID)painter, NULL, NULL);
	}

	typedef void(__cdecl* StartHookProc)(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans, int64_t echo);
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

	void Hook(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans, int64_t echo)
	{
		if (auto h = LoadHookLibrary()) {
			auto proc = (StartHookProc)GetProcAddress(h, "StartClickEffect");
			if (proc) proc(color, radius, width, delay, trans, echo);
		}
	}

	void Unhook()
	{
		if (auto h = LoadHookLibrary()) {
			auto proc = (StopHookProc)GetProcAddress(h, "StopClickEffect");
			if (proc) proc();
		}
	}

	void Show()
	{
		auto hWnd = ::FindWindow(wsHookerName, NULL);
		if (hWnd) ::SendMessage(hWnd, WM_SHOW_CLICK, 0, 0);
	}

}

extern "C" {
	using namespace ClickEffect;

	__declspec(dllexport) void __cdecl StopClickEffect()
	{
		auto hWnd = FindWindow(wsHookerName, NULL);
		if (hWnd) PostMessage(hWnd, WM_DESTROY, 0, 0);
		if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
	}

	__declspec(dllexport) void __cdecl StartClickEffect(int64_t color, int64_t radius, int64_t width, int64_t delay, int64_t trans, int64_t echo)
	{
		StopClickEffect();
		Hooker* settings = new Hooker(color, radius, width, delay, trans, echo);
		CreateThread(0, NULL, HookerThreadProc, (LPVOID)settings, NULL, NULL);
	}
}

#endif //_WINDOWS