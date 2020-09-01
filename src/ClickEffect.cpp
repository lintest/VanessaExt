#ifdef _WINDOWS

#include "stdafx.h"
#include "windows.h"
#include "ClickEffect.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

struct ClickData {
	HMODULE hModule;
	COLORREF color;
	int x, y, r;
};

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	ClickData &p = *(ClickData*)lpParam;

	HINSTANCE hModule = p.hModule;
	int x = p.x - p.r - 1;
	int y = p.y - p.r - 1;
	int w = p.r * 2 + 2;
	int h = p.r * 2 + 1;

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
	DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST;
	HWND hWnd = CreateWindowEx(dwExStyle, name, name, dwStyle, x, y, w, h, 0, 0, hModule, 0);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpParam);

	MSG msg;
	const int ID_Timer1 = 1;
	SetTimer(hWnd, ID_Timer1, 20, NULL);
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 127, LWA_COLORKEY | LWA_ALPHA);
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	delete (ClickData*)lpParam;
	return 0;
}

LRESULT CALLBACK PaintWindow(HWND hWnd)
{
	ClickData data = *(ClickData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	static int step = 0;
	static int count = 30;
	static int radius = 30;
	int width = 10 * (count - step) / count;
	HPEN redPen = CreatePen(PS_SOLID, width, data.color);
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	HPEN OldPen = (HPEN)SelectObject(hdc, redPen);
	HBRUSH OldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(BLACK_BRUSH));
	int r = step * radius / count;
	Ellipse(hdc, 30 - r, 30 - r, 30 + r, 30 + r);
	SelectObject(hdc, OldBrush);
	SelectObject(hdc, OldPen);
	EndPaint(hWnd, &ps);
	step++;
	if (step > count) {
		step = 0;
		radius = 30;
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
	case WM_PAINT:
		return PaintWindow(hWnd);
	case WM_TIMER:
		InvalidateRect(hWnd, NULL, TRUE);
		return 0;;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

HANDLE ClickEffect::Show(int64_t x, int64_t y)
{
	POINT pos;
	::GetCursorPos(&pos);
	ClickData* data = new ClickData;
	data->hModule = this->hModule;
	data->color = RGB(200, 50, 50);
	data->x = pos.x;
	data->y = pos.y;
	data->r = 30;

	return CreateThread(0, NULL, ThreadProc, (LPVOID)data, NULL, NULL);
}

#endif //_WINDOWS