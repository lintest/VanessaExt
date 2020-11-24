#ifdef _WINDOWS

#include "stdafx.h"
#include "windows.h"
#include "VideoPainter.h"

class RectPainter
	: public PainterBase {
public:
	RectPainter(const VideoPainter &p, int x, int y, int w, int h)
		: PainterBase(p) 
	{
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	}
	virtual LRESULT paint(HWND hWnd) override;
};

class EllipsePainter
	: public PainterBase {
public:
	EllipsePainter(const VideoPainter& p, int x, int y, int w, int h)
		: PainterBase(p)
	{
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	}
	virtual LRESULT paint(HWND hWnd) override;
};

#define ID_CLICK_TIMER 1

PainterBase* VideoPainter::painter(HWND hWnd)
{
	return (PainterBase*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

PainterBase* VideoPainter::painter(LPVOID lpParam)
{
	return (PainterBase*)lpParam;
}

LRESULT EllipsePainter::paint(HWND hWnd)
{
	int z = m_thick / 2;
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	HPEN pen = CreatePen(PS_SOLID, m_thick, m_color);
	HPEN hOldPen = (HPEN)SelectObject(hdc, pen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(BLACK_BRUSH));
	Ellipse(hdc, z, z, w - 2 * z, h - 2 * z);
	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	EndPaint(hWnd, &ps);
	DeleteObject(pen);
	return 0L;
}

LRESULT RectPainter::paint(HWND hWnd)
{
	int z = m_thick / 2;
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	HPEN pen = CreatePen(PS_SOLID, m_thick, m_color);
	HPEN hOldPen = (HPEN)SelectObject(hdc, pen);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(BLACK_BRUSH));
	POINT points[4] = {
		{z, z},
		{w - 2 * z, z},
		{w - 2 * z, h - 2 * z},
		{z, h - 2 * z},
	};
	Polygon(hdc, points, 4);
	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	EndPaint(hWnd, &ps);
	DeleteObject(pen);
	return 0L;
}

static LRESULT CALLBACK PainterWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
			KillTimer(hWnd, ID_CLICK_TIMER);
			SendMessage(hWnd, WM_DESTROY, 0, 0);
		}
		return 0;
	case WM_PAINT:
		return VideoPainter::painter(hWnd)->paint(hWnd);
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

void PainterBase::create()
{
	LPCWSTR name = L"VanessaVideoPainter";
	WNDCLASS wndClass;
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = PainterWndProc;
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

	SetTimer(hWnd, ID_CLICK_TIMER, m_delay, NULL);
	SetLayeredWindowAttributes(hWnd, 0, m_trans, LWA_COLORKEY | LWA_ALPHA);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);
}

static DWORD WINAPI PainterThreadProc(LPVOID lpParam)
{
	auto painter = VideoPainter::painter(lpParam);
	painter->create();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	delete painter;
	return 0;
}

void VideoPainter::ellipse(int left, int top, int width, int height)
{
	auto painter = new EllipsePainter(*this, (int)left, (int)top, (int)width, (int)height);
	CreateThread(0, NULL, PainterThreadProc, (LPVOID)painter, NULL, NULL);
}

void VideoPainter::rect(int left, int top, int width, int height)
{
	auto painter = new RectPainter(*this, (int)left, (int)top, (int)width, (int)height);
	CreateThread(0, NULL, PainterThreadProc, (LPVOID)painter, NULL, NULL);
}

#endif //_WINDOWS