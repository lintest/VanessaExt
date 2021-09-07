#ifdef _WINDOWS

#include "EducationShow.h"
#include "WindowsManager.h"
#include "windowsx.h"

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

static Color makeTransparent(const Color& color, int trans) {
	return Color(trans & 0xFF, color.GetRed(), color.GetGreen(), color.GetBlue());
}

EducationShow::EducationShow(const std::string& p, const std::wstring& title, const std::wstring& button)
	: title(title), button(button)
{
	JSON j = JSON::parse(p);
	get(j, "color", color);
	get(j, "padding", padding);
	get(j, "duration", duration);
	get(j, "frameCount", limit);
	get(j, "frameDelay", delay);
	get(j, "thickness", thick);
	get(j, "transparency", trans);
	get(j, "fontName", fontName);
	get(j, "fontSize", fontSize);
	get(j, "fontColor", fontColor);
	get(j, "background", background);

	if (this->title.empty()) get(j, "title", this->title);
	if (this->button.empty()) get(j, "button", this->button);

	if (trans) {
		color = makeTransparent(color, trans);
		fontColor = makeTransparent(fontColor, trans);
		background = makeTransparent(background, trans);
	}

	auto pid = GetCurrentProcessId();
	HWND hMainWnd = (HWND)WindowsManager::GetMainProcessWindow(pid);

	MONITORINFO mi{ 0 };
	mi.cbSize = sizeof(MONITORINFO);
	auto hMonitor = MonitorFromWindow(hMainWnd, MONITOR_DEFAULTTONEAREST);
	GetMonitorInfo(hMonitor, &mi);
	RECT mr = mi.rcMonitor;

	auto hDC = GetDC(NULL);
	Graphics graphics(hDC);
	FontFamily fontFamily(fontName.c_str());
	Font font(&fontFamily, fontSize, FontStyleRegular, UnitPoint);
	StringFormat format;
	format.SetAlignment(StringAlignment::StringAlignmentCenter);
	format.SetLineAlignment(StringAlignment::StringAlignmentCenter);
	Gdiplus::RectF rect((REAL)mr.left, (REAL)mr.top, (REAL)(mr.right - mr.left), (REAL)(mr.bottom - mr.top)), titleRect, buttonRect;
	graphics.MeasureString((WCHAR*)title.c_str(), (int)title.size(), &font, rect, &format, &titleRect);
	graphics.MeasureString((WCHAR*)button.c_str(), (int)button.size(), &font, rect, &format, &buttonRect);
	titleWidth = (int)titleRect.Width + thick * 2;
	buttonWidth = (int)buttonRect.Width + thick * 2;

	this->x = (mr.left + mr.right - buttonWidth - titleWidth) / 2 - thick;
	this->y = mr.top;
	this->w = titleWidth + buttonWidth + thick * 2;
	this->h = (int)max(titleRect.Height, buttonRect.Height) + thick * 2;
}

void EducationShow::draw(Graphics& graphics, bool hover)
{
	if (hover || moving) {
		color = makeTransparent(color, 255);
		fontColor = makeTransparent(fontColor, 255);
		background = makeTransparent(background, 255);
		buttonColor = makeTransparent(buttonColor, 255);
	}
	else {
		color = makeTransparent(color, trans);
		fontColor = makeTransparent(fontColor, trans);
		background = makeTransparent(background, trans);
		buttonColor = makeTransparent(buttonColor, trans);
	}

	SolidBrush brush(background);
	GraphicsPath path;
	path.AddRectangle(Rect(0, 0, w, h));
	graphics.FillPath(&brush, &path);

	SolidBrush buttonBrush(buttonColor);
	GraphicsPath buttonPath;
	buttonPath.AddRectangle(Rect(w - buttonWidth, 0, buttonWidth, h));
	graphics.FillPath(&buttonBrush, &buttonPath);

	Pen pen(color, (REAL)thick);
	graphics.DrawPath(&pen, &path);

	SolidBrush textBrush(fontColor);
	FontFamily fontFamily(fontName.c_str());
	Font font(&fontFamily, fontSize, FontStyleRegular, UnitPoint);
	StringFormat format;
	format.SetAlignment(StringAlignment::StringAlignmentCenter);
	format.SetLineAlignment(StringAlignment::StringAlignmentCenter);

	RectF titleRect((REAL)0, (REAL)0, (REAL)titleWidth, (REAL)h);
	graphics.DrawString((WCHAR*)title.c_str(), (int)title.size(), &font, titleRect, &format, &textBrush);

	SolidBrush buttonTextBrush(Color::White);
	RectF buttonRect((REAL)titleWidth, (REAL)0, (REAL)w - titleWidth, (REAL)h);
	graphics.DrawString((WCHAR*)button.c_str(), (int)button.size(), &font, buttonRect, &format, &buttonTextBrush);
}

LRESULT EducationShow::repaint(HWND hWnd, bool hover)
{
	GgiPlusToken::Init();
	Bitmap bitmap(w, h, PixelFormat32bppARGB);
	Graphics graphics(&bitmap);
	graphics.Clear(Color::Transparent);
	graphics.SetCompositingQuality(CompositingQuality::CompositingQualityHighQuality);
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.SetTextRenderingHint(TextRenderingHint::TextRenderingHintAntiAlias);
	draw(graphics, hover);

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
	RECT rect;
	GetWindowRect(hWnd, &rect);
	SIZE size = { w, h };
	POINT ptDst = { rect.left, rect.top };
	POINT ptSrc = { 0, 0 };
	UpdateLayeredWindow(hWnd, hDC, &ptDst, &size, hCDC, &ptSrc, 0, &bf, ULW_ALPHA);

	DeleteObject(hBitmap);
	DeleteDC(hCDC);
	ReleaseDC(hWnd, hDC);
	return 0;
}

static EducationShow& win(HWND hWnd)
{
	return *(EducationShow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

static EducationShow& win(LPARAM lParam)
{
	return *((EducationShow*)((CREATESTRUCT*)lParam)->lpCreateParams);
}

static void TrackMouse(HWND hWnd) 
{
	TRACKMOUSEEVENT csTME{ 0 };
	csTME.cbSize = sizeof(csTME);
	csTME.dwFlags = TME_LEAVE | TME_HOVER | TME_NONCLIENT;
	csTME.hwndTrack = hWnd;
	csTME.dwHoverTime = 10;
	::TrackMouseEvent(&csTME);
}

LRESULT EducationShow::onMouseDown(HWND hWnd, LPARAM lParam)
{
	moving = true;
	mx = GET_X_LPARAM(lParam);
	my = GET_Y_LPARAM(lParam);
	return 0;
}

LRESULT EducationShow::onMouseMove(HWND hWnd, LPARAM lParam)
{
	win(hWnd).repaint(hWnd, true);
	TrackMouse(hWnd);
	return 0;
}

LRESULT EducationShow::onMouseUp(HWND hWnd, LPARAM lParam)

{
	moving = false;
	return 0;
}

LRESULT EducationShow::onHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
/*
	POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	::ScreenToClient(hWnd, &point);
	const LRESULT result = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	return (result == HTCLIENT && point.x < titleWidth) ? HTCAPTION : result;
*/
	const LRESULT result = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	return (result == HTCLIENT) ? HTCAPTION : result;

}

static LRESULT CALLBACK PainterWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCREATE: {
		LPCREATESTRUCT lpcp = (LPCREATESTRUCT)lParam;
		lpcp->style &= (~WS_CAPTION);
		lpcp->style &= (~WS_BORDER);
		SetWindowLong(hWnd, GWL_STYLE, lpcp->style);
		return TRUE;
	}
	case WM_NCHITTEST:
		return win(hWnd).onHitTest(hWnd, uMsg, wParam, lParam);
	case WM_CREATE:
		return win(lParam).repaint(hWnd, false);
	case WM_LBUTTONDOWN:
		return win(hWnd).onMouseDown(hWnd, lParam);
	case WM_LBUTTONUP:
		return win(hWnd).onMouseUp(hWnd, lParam);
	case WM_NCMOUSEMOVE:
		return win(hWnd).onMouseMove(hWnd, lParam);
	case WM_NCMOUSEHOVER:
		return win(hWnd).repaint(hWnd, true);
	case WM_NCMOUSELEAVE:
		return win(hWnd).repaint(hWnd, false);
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

const LPCWSTR wsEducationClass = L"VanessaEducationShow";

void EducationShow::create()
{
	WNDCLASS wndClass = {};
	wndClass.lpfnWndProc = PainterWndProc;
	wndClass.hInstance = hModule;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszClassName = wsEducationClass;
	RegisterClass(&wndClass);

	DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
	HWND hWnd = CreateWindowEx(dwExStyle, wsEducationClass, NULL, WS_POPUP, x, y, w, h, NULL, NULL, hModule, this);
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
	HWND hWnd = ::FindWindow(wsEducationClass, NULL);
	if (hWnd) ::SendMessage(hWnd, WM_DESTROY, 0, 0);
	CreateThread(0, NULL, PainterThreadProc, (LPVOID)this, NULL, NULL);
}

#endif //_WINDOWS