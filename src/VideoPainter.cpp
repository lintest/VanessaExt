#ifdef _WINDOWS

#include "stdafx.h"
#include "windows.h"
#include "VideoPainter.h"
#include "ImageHelper.h"

#define ID_CLICK_TIMER 1

using namespace Gdiplus;

PainterBase* VideoPainter::painter(HWND hWnd)
{
	return (PainterBase*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

LRESULT RecanglePainter::paint(HWND hWnd)
{
	int z = thick / 2;
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	HPEN pen = CreatePen(PS_SOLID, thick, color);
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

LRESULT EllipsePainter::paint(HWND hWnd)
{
	int z = thick / 2;
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	Gdiplus::Color color;
	color.SetFromCOLORREF(this->color);
	Pen pen(color, thick);
	Gdiplus::Graphics graphics(hdc);
	graphics.Clear(Gdiplus::Color::Transparent);
	graphics.SetCompositingQuality(CompositingQualityHighQuality);
//	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.DrawEllipse(&pen, z, z, w - 2 * z, h - 2 * z);
	EndPaint(hWnd, &ps);
	return 0L;
}

PolyBezierPainter::PolyBezierPainter(const VideoPainter& p, const std::string& text)
	: PainterBase(p)
{
	try {
		auto list = JSON::parse(text);
		for (size_t i = 0; i < list.size(); i++) {
			auto item = list[i];
			points.push_back({ item["x"], item["y"] });
		}
		if (list.size() == 0) throw 0;
		auto p = points[0];
		int left = p.X, top = p.Y, right = p.X, bottom = p.Y;
		for (auto it = points.begin() + 1; it != points.end(); ++it) {
			if (left > it->X) left = it->X;
			if (top > it->Y) top = it->Y;
			if (right < it->X) right = it->X;
			if (bottom < it->Y) bottom = it->Y;
		}
		x = left - thick;
		y = top - thick;
		w = right - left + thick * 2;
		h = bottom - top + thick * 2;
		for (auto it = points.begin(); it != points.end(); ++it) {
			it->X -= x;
			it->Y -= y;
		}
		GgiPlusToken::Init();
	}
	catch (...) {
		throw std::u16string(u"JSON parsing error");
	}
	start();
}

LRESULT PolyBezierPainter::paint(HWND hWnd)
{
	int z = thick / 2;
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	Gdiplus::Color color;
	color.SetFromCOLORREF(this->color);
	Pen pen(color, thick);
	Graphics graphics(hdc);
	graphics.Clear(Gdiplus::Color::Transparent);
	graphics.SetCompositingQuality(CompositingQualityHighQuality);
//	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.DrawBeziers(&pen, points.data(), (INT)points.size());
	EndPaint(hWnd, &ps);
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

	SetTimer(hWnd, ID_CLICK_TIMER, delay, NULL);
	SetLayeredWindowAttributes(hWnd, 0, trans, LWA_COLORKEY | LWA_ALPHA);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);
}

static DWORD WINAPI PainterThreadProc(LPVOID lpParam)
{
	auto painter = (PainterBase*)lpParam;
	painter->create();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	delete painter;
	return 0;
}

void PainterBase::start()
{
	CreateThread(0, NULL, PainterThreadProc, (LPVOID)this, NULL, NULL);
}

#endif //_WINDOWS