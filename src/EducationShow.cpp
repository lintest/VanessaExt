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

bool EducationShow::sm_stop = false;

EducationShow::EducationShow(AddInNative& addin, const std::string& p, const std::wstring& title, const std::wstring& button, const std::wstring& filename)
	: addin(addin), title(title), button(button), filename(filename)
{
	JSON j = JSON::parse(p);
	get(j, "color", color);
	get(j, "padding", padding);
	get(j, "eventName", eventName);
	get(j, "eventData", eventData);
	get(j, "thickness", thick);
	get(j, "transparency", trans);
	get(j, "fontName", fontName);
	get(j, "fontSize", fontSize);
	get(j, "fontColor", fontColor);
	get(j, "background", background);
	get(j, "buttonColor", buttonColor);
	get(j, "iconColor", iconColor);
	get(j, "identifier", identifier);

	if (this->title.empty()) get(j, "title", this->title);
	if (this->button.empty()) get(j, "button", this->button);

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
	std::wstring text = L"X" + button + L"X";
	graphics.MeasureString((WCHAR*)text.c_str(), (int)text.size(), &font, rect, &format, &buttonRect);
	titleWidth = (int)titleRect.Width + thick * 2;
	buttonHeight = (int)titleRect.Height + thick * 2;
	buttonIconWidth = (int)titleRect.Height + thick * 2;
	buttonTextWidth = (int)buttonRect.Width + thick * 2;
	buttonWidth = buttonIconWidth + buttonTextWidth;

	this->x = (mr.left + mr.right - buttonWidth - titleWidth) / 2 - thick;
	this->y = mr.top;
	this->w = titleWidth + buttonWidth + thick * 2;
	this->h = (int)max(titleRect.Height, buttonRect.Height) + thick * 2;
}

void EducationShow::draw(Graphics& graphics)
{
	if (hover) {
		color = makeTransparent(color, 255);
		fontColor = makeTransparent(fontColor, 255);
		background = makeTransparent(background, 255);
		buttonColor = makeTransparent(buttonColor, 255);
		stopColor = makeTransparent(stopColor, 255);
		iconColor = makeTransparent(iconColor, 255);
	}
	else {
		color = makeTransparent(color, trans);
		fontColor = makeTransparent(fontColor, trans);
		background = makeTransparent(background, trans);
		buttonColor = makeTransparent(buttonColor, trans);
		stopColor = makeTransparent(stopColor, trans);
		iconColor = makeTransparent(iconColor, trans);
	}

	SolidBrush brush(background);
	GraphicsPath path;
	path.AddRectangle(Rect(0, 0, w, h));
	graphics.FillPath(&brush, &path);

	REAL z = buttonIconWidth / 4;
	REAL dy = pressed ? max(buttonIconWidth / 12, 2) : 0;

	Color btnColor = buttonColor;
	if (pressed) btnColor = Color(color.GetAlpha(), color.GetRed() * 0.8, color.GetGreen() * 0.8, color.GetBlue() * 0.8);
	SolidBrush buttonBrush(btnColor);
	GraphicsPath buttonPath;
	buttonPath.AddRectangle(Rect(w - buttonWidth, 0, buttonWidth, h));
	graphics.FillPath(&buttonBrush, &buttonPath);

	SolidBrush iconBrush(iconColor);
	GraphicsPath iconPath;
	iconPath.AddRectangle(Rect(w - buttonWidth + z * 2, z + dy, z * 2, z * 2));
	graphics.FillPath(&iconBrush, &iconPath);

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

	SolidBrush stopBrush(stopColor);
	RectF buttonRect((REAL)w - buttonTextWidth, (REAL)dy, (REAL)buttonTextWidth, (REAL)h);
	graphics.DrawString((WCHAR*)button.c_str(), (int)button.size(), &font, buttonRect, &format, &stopBrush);
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

LRESULT EducationShow::onMouseDown(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	::ScreenToClient(hWnd, &point);
	if (point.x > titleWidth) {
		hover = true;
		pressed = true;
		win(hWnd).repaint(hWnd);
		TrackMouse(hWnd);
		return 0;
	}
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT EducationShow::onMouseMove(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TrackMouse(hWnd);
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT EducationShow::onMouseUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (pressed) {
		pressed = false;
		win(hWnd).repaint(hWnd);
		EducationShow::sm_stop = true;
		addin.ExternalEvent((char16_t*)eventName.c_str(), (char16_t*)eventData.c_str());
		if (!filename.empty())
		{
			auto hFile = CreateFileW(filename.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				WCHAR buf[32];
				SYSTEMTIME st;
				GetLocalTime(&st);
				wsprintfW(buf, L"%.4u-%.2u-%.2uT%.2u:%.2u:%.2u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
				WriteFile(hFile, buf, wcslen(buf) * sizeof(WCHAR), NULL, NULL);
				CloseHandle(hFile);
			}
		}
	}
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT EducationShow::onHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const LRESULT result = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	return (result == HTCLIENT) ? HTCAPTION : result;
}

LRESULT EducationShow::onMouseHover(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	hover = true;
	win(hWnd).repaint(hWnd);
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT EducationShow::onMouseLeave(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	hover = false;
	pressed = false;
	win(hWnd).repaint(hWnd);
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
	case WM_CREATE:
		return win(lParam).repaint(hWnd);
	case WM_NCHITTEST:
		return win(hWnd).onHitTest(hWnd, uMsg, wParam, lParam);
	case WM_NCLBUTTONDOWN:
		return win(hWnd).onMouseDown(hWnd, uMsg, wParam, lParam);
	case WM_NCLBUTTONUP:
		return win(hWnd).onMouseUp(hWnd, uMsg, wParam, lParam);
	case WM_NCMOUSEMOVE:
		return win(hWnd).onMouseMove(hWnd, uMsg, wParam, lParam);
	case WM_NCMOUSEHOVER:
		return win(hWnd).onMouseHover(hWnd, uMsg, wParam, lParam);
	case WM_NCMOUSELEAVE:
		return win(hWnd).onMouseLeave(hWnd, uMsg, wParam, lParam);
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

const LPCWSTR wsEducationClass = L"VanessaTerminator";

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
	HWND hWnd = CreateWindowEx(dwExStyle, wsEducationClass, identifier.c_str(), WS_POPUP, x, y, w, h, NULL, NULL, hModule, this);
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

void EducationShow::close()
{
	while (HWND hWnd = ::FindWindow(wsEducationClass, NULL)) {
		::SendMessage(hWnd, WM_DESTROY, 0, 0);
	}
}

void EducationShow::run()
{
	close();
	sm_stop = false;
	CreateThread(0, NULL, PainterThreadProc, (LPVOID)this, NULL, NULL);
}

#endif //_WINDOWS