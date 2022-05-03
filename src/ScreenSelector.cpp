#ifdef _WINDOWS

#include "ScreenSelector.h"
#include "WindowsManager.h"
#include "windowsx.h"

static Color makeTransparent(const Color& color, int trans) {
	return Color(trans & 0xFF, color.GetRed(), color.GetGreen(), color.GetBlue());
}

ScreenSelector::ScreenSelector(AddInNative& addin, const std::string& p, const std::wstring& title)
	: addin(addin), title(title)
{
	JSON j = JSON::parse(p);

	auto pid = GetCurrentProcessId();
	HWND hMainWnd = (HWND)WindowsManager::GetMainProcessWindow(pid);

	MONITORINFO mi{ 0 };
	mi.cbSize = sizeof(MONITORINFO);
	auto hMonitor = MonitorFromWindow(hMainWnd, MONITOR_DEFAULTTONEAREST);
	GetMonitorInfo(hMonitor, &mi);
	RECT mr = mi.rcMonitor;
	this->x = mr.left;
	this->y = mr.top;
	this->w = mr.right - mr.left;
	this->h = mr.bottom - mr.top;
}
/*
void ScreenSelector::draw(Graphics& graphics)
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

	auto xx = min(m_start.x, m_finish.x);
	auto yy = min(m_start.y, m_finish.y);
	auto ww = min(m_start.x, m_finish.x);
	auto hh = max(m_start.y, m_finish.y);

	SolidBrush brush(background);
	GraphicsPath path;
	path.AddRectangle(Rect(xx, yy, ww, hh));
	graphics.FillPath(&brush, &path);
	Pen pen(color, (REAL)thick);
	graphics.DrawPath(&pen, &path);
}

*/

void ScreenSelector::draw(Graphics& graphics)
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

	auto xx = min(m_start.x, m_finish.x);
	auto yy = min(m_start.y, m_finish.y);
	auto ww = max(m_start.x, m_finish.x) - xx;
	auto hh = max(m_start.y, m_finish.y) - yy;

	SolidBrush brush1(fontColor);
	GraphicsPath path1;
	path.AddRectangle(Rect(xx, yy, ww, hh));
	graphics.FillPath(&brush1, &path1);
	Pen pen(color, (REAL)thick);
	graphics.DrawPath(&pen, &path);
}

LRESULT ScreenSelector::repaint(HWND hWnd)
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

static ScreenSelector& win(HWND hWnd)
{
	return *(ScreenSelector*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

static ScreenSelector& win(LPARAM lParam)
{
	return *((ScreenSelector*)((CREATESTRUCT*)lParam)->lpCreateParams);
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

LRESULT ScreenSelector::onMouseDown(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	m_start = POINT { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	m_pressed = true;
	TrackMouse(hWnd);
	return 0;
}

LRESULT ScreenSelector::onMouseMove(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	m_finish = POINT{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	TrackMouse(hWnd);
	win(hWnd).repaint(hWnd);
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT ScreenSelector::onMouseUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_pressed) {
		auto left = min(m_start.x, m_finish.x);
		auto top = min(m_start.y, m_finish.y);
		auto rigth = max(m_start.x, m_finish.x);
		auto bottom = max(m_start.y, m_finish.y);
		JSON json = {
			{ "left", left },
			{ "top", top },
			{ "width", rigth - left },
			{ "height", bottom - top },
		};
		m_pressed = false;
		auto data = MB2WCHAR(json.dump());
		addin.ExternalEvent((char16_t*)m_event.c_str(), (char16_t*)data.c_str());
		::SendMessage(hWnd, WM_DESTROY, 0, 0);
	}
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT ScreenSelector::onHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const LRESULT result = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	return (result == HTCLIENT) ? HTCAPTION : result;
}

static LRESULT CALLBACK SelectorWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

const LPCWSTR wsSelectorClass = L"VanessaScreenSelector";

void ScreenSelector::create()
{
	WNDCLASS wndClass = {};
	wndClass.lpfnWndProc = SelectorWndProc;
	wndClass.hInstance = hModule;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszClassName = wsSelectorClass;
	RegisterClass(&wndClass);

	DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
	HWND hWnd = CreateWindowEx(dwExStyle, wsSelectorClass, identifier.c_str(), WS_POPUP, x, y, w, h, NULL, NULL, hModule, this);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);
}

static DWORD WINAPI SelectorThreadProc(LPVOID lpParam)
{
	std::unique_ptr<ScreenSelector> show((ScreenSelector*)lpParam);
	show->create();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

void ScreenSelector::close()
{
	while (HWND hWnd = ::FindWindow(wsSelectorClass, NULL)) {
		::SendMessage(hWnd, WM_DESTROY, 0, 0);
	}
}

void ScreenSelector::run()
{
	close();
	CreateThread(0, NULL, SelectorThreadProc, (LPVOID)this, NULL, NULL);
}

#endif //_WINDOWS