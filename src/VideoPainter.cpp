#ifdef _WINDOWS

#include "VideoPainter.h"

#define ID_PAINTER_TIMER 1

using namespace Gdiplus;

void RecanglePainter::paint(Gdiplus::Graphics& graphics)
{
	int z = thick / 2;
	Gdiplus::Color color;
	color.SetFromCOLORREF(this->color);
	Pen pen(color, (REAL)thick);
	Point points[4] = {
		{z, z},
		{w - 2 * z, z},
		{w - 2 * z, h - 2 * z},
		{z, h - 2 * z},
	};
	graphics.DrawPolygon(&pen, points, 4);
}

void EllipsePainter::paint(Gdiplus::Graphics& graphics)
{
	Gdiplus::Color color;
	color.SetFromCOLORREF(this->color);
	Pen pen(color, (REAL)thick);
	graphics.DrawEllipse(&pen, thick, thick, w - 2 * thick, h - 2 * thick);
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
		x = left - 2 * thick;
		y = top - 2 * thick;
		w = right - left + 4 * thick;
		h = bottom - top + 4 * thick;
		for (auto it = points.begin(); it != points.end(); ++it) {
			it->X -= x;
			it->Y -= y;
		}
		createThread();
	}
	catch (...) {
		throw std::u16string(u"JSON parsing error");
	}
}

void PolyBezierPainter::paint(Gdiplus::Graphics& graphics)
{
	REAL z = (REAL)thick;
	Gdiplus::Color color;
	color.SetFromCOLORREF(this->color);
	Gdiplus::Pen pen(color, z);
	AdjustableArrowCap arrow(z * 2, z);
	pen.SetStartCap(LineCapRoundAnchor);
	pen.SetCustomEndCap(&arrow);
	graphics.DrawBeziers(&pen, points.data(), (INT)points.size());
}

void ArrowPainter::paint(Gdiplus::Graphics& graphics)
{
	REAL z = (REAL)thick;
	Gdiplus::Color color;
	color.SetFromCOLORREF(this->color);
	Gdiplus::Pen pen(color, z);
	AdjustableArrowCap arrow(z * 2, z);
	pen.SetStartCap(LineCapRoundAnchor);
	pen.SetCustomEndCap(&arrow);
	graphics.DrawLine(&pen, x1 - x, y1 - y, x2 - x, y2 - y);
}

LRESULT PainterBase::create(HWND hWnd)
{
	GgiPlusToken::Init();
	Bitmap bitmap(w, h, PixelFormat32bppARGB);
	Gdiplus::Graphics graphics(&bitmap);
	graphics.Clear(Gdiplus::Color::Transparent);
	graphics.SetCompositingQuality(CompositingQualityHighQuality);
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	paint(graphics);

	//Инициализируем составляющие временного DC, в который будет отрисована маска
	auto hDC = GetDC(hWnd);
	auto hCDC = CreateCompatibleDC(hDC);

	BITMAPINFO bi = {};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biWidth = w;
	bi.bmiHeader.biHeight = h;
	bi.bmiHeader.biPlanes = 1;
	auto hBitmap = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, NULL, NULL, 0);
	if (!hBitmap) return -1;

	SelectObject(hCDC, hBitmap);
	//Создаем объект Graphics на основе контекста окна
	Graphics window(hCDC);
	//Рисуем маску на окне
	window.DrawImage(&bitmap, 0, 0, w, h);

	//В параметрах ULW определяем, что в качестве значения полупрозрачности будет использоваться
	//альфа-компонент пикселей исходного изображения
	BLENDFUNCTION bf = {};
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 255;

	//Применяем отрисованную маску с альфой к окну
	SIZE size = { w, h };
	POINT ptDst = { x, y };
	POINT ptSrc = { 0, 0 };
	UpdateLayeredWindow(hWnd, hDC, &ptDst, &size, hCDC, &ptSrc, 0, &bf, ULW_ALPHA);

	DeleteObject(hBitmap);
	DeleteDC(hCDC);
	ReleaseDC(hWnd, hDC);
	return 0;
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
		return TRUE;
	}
	case WM_CREATE:
		return ((VideoPainter*)((CREATESTRUCT*)lParam)->lpCreateParams)->create(hWnd);
	case WM_TIMER:
		if (wParam == ID_PAINTER_TIMER) {
			KillTimer(hWnd, ID_PAINTER_TIMER);
			SendMessage(hWnd, WM_DESTROY, 0, 0);
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

void PainterBase::createWindow()
{
	LPCWSTR name = L"VanessaVideoPainter";
	WNDCLASS wndClass = {};
	wndClass.lpfnWndProc = PainterWndProc;
	wndClass.hInstance = hModule;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszClassName = name;
	RegisterClass(&wndClass);

	DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
	HWND hWnd = CreateWindowEx(dwExStyle, name, name, WS_POPUP, x, y, w, h, NULL, NULL, hModule, this);
	SetTimer(hWnd, ID_PAINTER_TIMER, delay, NULL);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);
}

static DWORD WINAPI PainterThreadProc(LPVOID lpParam)
{
	auto painter = (PainterBase*)lpParam;
	painter->createWindow();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	delete painter;
	return 0;
}

void PainterBase::createThread()
{
	CreateThread(0, NULL, PainterThreadProc, (LPVOID)this, NULL, NULL);
}

#endif //_WINDOWS