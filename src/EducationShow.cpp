#ifdef _WINDOWS

#include "EducationShow.h"

static void get(JSON& j, const std::string& name, Color& value)
{
	auto it = j.find(name);
	if (it != j.end())
		value.SetFromCOLORREF(*it);
}

static void get(JSON& j, const std::string& name, std::wstring& value)
{
	auto it = j.find(name);
	if (it != j.end())
		value = MB2WC(*it);
}

template<typename T>
static void get(JSON& j, const std::string& name, T& value)
{
	auto it = j.find(name);
	if (it != j.end())
		value = *it;
}

EducationShow::EducationShow(const std::string& p, int x, int y, const std::wstring& t)
	: text(t)
{
	int trans = 0;
	JSON j = JSON::parse(p);
	get(j, "color", color);
	get(j, "duration", duration);
	get(j, "frameCount", limit);
	get(j, "frameDelay", delay);
	get(j, "thickness", thick);
	get(j, "transparency", trans);
	get(j, "fontName", fontName);
	get(j, "fontSize", fontSize);
	if (text.empty()) get(j, "text", text);
	if (trans) color = Color(trans & 0xFF, color.GetRed(), color.GetGreen(), color.GetBlue());

	MONITORINFO mi{ 0 };
	mi.cbSize = sizeof(MONITORINFO);
	auto hMonitor = MonitorFromPoint(POINT{ x, y }, MONITOR_DEFAULTTONEAREST);
	GetMonitorInfo(hMonitor, &mi);
	RECT mr = mi.rcMonitor;

	auto hDC = GetDC(NULL);
	Graphics graphics(hDC);
	FontFamily fontFamily(fontName.c_str());
	Font font(&fontFamily, fontSize, FontStyleRegular, UnitPoint);
	StringFormat format;
	format.SetAlignment(StringAlignment::StringAlignmentCenter);
	format.SetLineAlignment(StringAlignment::StringAlignmentCenter);
	Gdiplus::RectF rect((REAL)mr.left, (REAL)mr.top, (REAL)(mr.right - mr.left), (REAL)(mr.bottom - mr.top)), r;
	graphics.MeasureString((WCHAR*)text.c_str(), (int)text.size(), &font, rect, &format, &r);

	this->x = (mr.left + mr.right + w) / 2 - thick;
	this->y = mr.top;
	this->w = int(r.Width);
	this->h = int(r.Height);
}

void EducationShow::draw(Graphics& graphics)
{
	SolidBrush brush(background);
	GraphicsPath path;
	path.AddRectangle(Rect(0, 0, w, h));
	graphics.FillPath(&brush, &path);

	SolidBrush textBrush(fontColor);
	FontFamily fontFamily(fontName.c_str());
	Font font(&fontFamily, fontSize, FontStyleRegular, UnitPoint);
	StringFormat format;
	format.SetAlignment(StringAlignment::StringAlignmentCenter);
	format.SetLineAlignment(StringAlignment::StringAlignmentCenter);
	RectF rect((REAL)0, (REAL)0, (REAL)w, (REAL)h), r;
	graphics.DrawString((WCHAR*)text.c_str(), (int)text.size(), &font, r, &format, &textBrush);
}

LRESULT EducationShow::repaint(HWND hWnd)
{
	GgiPlusToken::Init();
	Bitmap bitmap(w, h, PixelFormat32bppARGB);
	Graphics graphics(&bitmap);
	graphics.Clear(Color::Transparent);
	graphics.SetCompositingQuality(CompositingQuality::CompositingQualityHighQuality);
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.SetTextRenderingHint(TextRenderingHint::TextRenderingHintAntiAlias);
	draw(graphics);

	//Инициализируем составляющие временного DC, в который будет отрисована маска
	auto hDC = GetDC(hWnd);
	auto hCDC = CreateCompatibleDC(hDC);

	LPVOID bits;
	BITMAPINFO bi = {};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biWidth = w;
	bi.bmiHeader.biHeight = h;
	bi.bmiHeader.biPlanes = 1;
	auto hBitmap = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
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
		return ((EducationShow*)((CREATESTRUCT*)lParam)->lpCreateParams)->repaint(hWnd);
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

const LPCWSTR name = L"VanessaEducationShow";

void EducationShow::create()
{
	WNDCLASS wndClass = {};
	wndClass.lpfnWndProc = PainterWndProc;
	wndClass.hInstance = hModule;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszClassName = name;
	RegisterClass(&wndClass);

	DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
	HWND hWnd = CreateWindowEx(dwExStyle, name, name, WS_POPUP, x, y, w, h, NULL, NULL, hModule, this);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);
}

static DWORD WINAPI PainterThreadProc(LPVOID lpParam)
{
	std::unique_ptr<EducationShow> show((EducationShow*)lpParam);
	show->create();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

void EducationShow::run()
{
	CreateThread(0, NULL, PainterThreadProc, (LPVOID)this, NULL, NULL);
}

#endif //_WINDOWS