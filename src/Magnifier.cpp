#ifdef _WINDOWS

#include "stdafx.h"
#include "Magnifier.h"

#include <magnification.h>
#pragma comment(lib, "magnification.lib")

static LRESULT CALLBACK MagnifierWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_NCCREATE: {
		LPCREATESTRUCT lpcp = (LPCREATESTRUCT)lParam;
		lpcp->style &= (~WS_CAPTION);
		lpcp->style &= (~WS_BORDER);
		SetWindowLong(hWnd, GWL_STYLE, lpcp->style);
		return TRUE;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

class MagnifierData {
public:
	float factor = 2.0f;
	int x, y, w, h, ww, hh, form;
	const UINT interval = 16;
	MagnifierData(int x, int y, int w, int h, float z, int f)
		: x(x), y(y), w(w), h(h), factor(z), form(f), ww(0), hh(0) {}
	HWND create();
};

static void CALLBACK MagnifierUpdateProc(HWND hMag, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	auto data = (MagnifierData*)GetWindowLongPtr(hMag, GWLP_USERDATA);

	POINT pos;
	GetCursorPos(&pos);

	int w = (int)(data->ww / data->factor);
	int h = (int)(data->hh / data->factor);
	int x = pos.x - w / 2;
	int y = pos.y - h / 2;
	x = max(x, 0); x = min(x, GetSystemMetrics(SM_CXSCREEN) - w);
	y = max(y, 0); y = min(y, GetSystemMetrics(SM_CYSCREEN) - h);
	RECT rect{ x, y, x + w, y + h };
	MagSetWindowSource(hMag, rect);
	SetWindowPos(hMag, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	InvalidateRect(hMag, NULL, TRUE);
}

LPCWSTR wsMagnifierName = L"VanessaMagnifierWindow";

void setRegion(HWND hWnd, int form, int x, int y, int w, int h)
{
	HRGN hRgn = 0;
	int r = min(w, h) / 8;
	switch (form) {
	case 1: hRgn = CreateRoundRectRgn(x, y, w, h, r, r); break;
	case 2: hRgn = CreateEllipticRgn(x, y, w, h); break;
	}
	if (hRgn) SetWindowRgn(hWnd, hRgn, FALSE);
}

HWND MagnifierData::create()
{
	WNDCLASS wndClass = {};
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = MagnifierWndProc;
	wndClass.hInstance = hModule;
	wndClass.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
	wndClass.lpszClassName = wsMagnifierName;
	RegisterClass(&wndClass);

	DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;
	HWND hWnd = CreateWindowEx(dwExStyle, wsMagnifierName, NULL, WS_POPUP, x, y, w, h, NULL, NULL, hModule, 0);
	if (!hWnd) return NULL;
	setRegion(hWnd, form, 0, 0, w, h);

	SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);

	RECT rect;
	GetClientRect(hWnd, &rect);
	int cx = GetSystemMetrics(SM_CXDLGFRAME);
	int cy = GetSystemMetrics(SM_CYDLGFRAME);
	rect.left += cx;
	rect.top += cy;
	ww = rect.right - rect.left - cx;
	hh = rect.bottom - rect.top - cy;
	DWORD dwMagStyle = WS_CHILD | MS_SHOWMAGNIFIEDCURSOR | WS_VISIBLE;
	HWND hMag = CreateWindow(WC_MAGNIFIER, NULL, dwMagStyle, rect.left, rect.top, ww, hh, hWnd, NULL, hModule, 0);
	setRegion(hMag, form, 0, 0, ww, hh);
	if (!hMag) return NULL;

	SetWindowLongPtr(hMag, GWLP_USERDATA, (LONG_PTR)this);

	MAGTRANSFORM matrix;
	memset(&matrix, 0, sizeof(matrix));
	matrix.v[0][0] = factor;
	matrix.v[1][1] = factor;
	matrix.v[2][2] = 1.0f;
	BOOL ret = MagSetWindowTransform(hMag, &matrix);

	return hMag;
}

static DWORD WINAPI MagnifierThreadProc(LPVOID lpParam)
{
	std::unique_ptr<MagnifierData> data((MagnifierData*)lpParam);
	if (FALSE == MagInitialize()) return 0;
	auto hMag = data->create();
	if (FALSE == hMag) return 0;

	UINT_PTR timerId = SetTimer(hMag, 0, data->interval, MagnifierUpdateProc);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	KillTimer(NULL, timerId);
	MagUninitialize();
	return (int)msg.wParam;
}

void Magnifier::Show(int x, int y, int w, int h, double z, int f)
{
	Hide();
	auto data = new MagnifierData(x, y, w, h, (float)z, f);
	CreateThread(0, NULL, MagnifierThreadProc, (LPVOID)data, NULL, NULL);
}

void Magnifier::Hide()
{
	auto hWnd = FindWindow(wsMagnifierName, NULL);
	if (hWnd) PostMessage(hWnd, WM_DESTROY, 0, 0);
}

#endif //_WINDOWS
