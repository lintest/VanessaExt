#ifdef _WINDOWS

#include "VideoPainter.h"

#define ID_TIMER_REPAINT 1
#define ID_TIMER_TIMEOUT 2

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

PainterBase::PainterBase(const std::string& params, int x, int y, int w, int h)
	: x(x), y(y), w(w), h(h)
{
	int trans = 0;
	JSON j = JSON::parse(params);
	get(j, "color", color);
	get(j, "duration", duration);
	get(j, "frameCount", limit);
	get(j, "frameDelay", delay);
	get(j, "thickness", thick);
	get(j, "transparency", trans);
	if (trans) color = Color(trans & 0xFF, color.GetRed(), color.GetGreen(), color.GetBlue());
	if (limit <= 0) limit = 1;
	if (delay == 0 || step > limit) step = limit;
}

void RecanglePainter::draw(Graphics& graphics)
{
	int z = thick / 2;
	Pen pen(color, (REAL)thick);
	Point points[4] = {
		{z, z},
		{w - 2 * z, z},
		{w - 2 * z, h - 2 * z},
		{z, h - 2 * z},
	};
	graphics.DrawPolygon(&pen, points, 4);
}

ShadowPainter::ShadowPainter(const std::string& p, int x, int y, int w, int h, const std::wstring& t)
	: PainterBase(p), X(x), Y(y), W(w), H(h), text(t)
{
	MONITORINFO mi{ 0 };
	mi.cbSize = sizeof(MONITORINFO);
	RECT rect{ x, y, x + w, y + h };
	auto hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	GetMonitorInfo(hMonitor, &mi);
	rect = mi.rcMonitor;
	X -= rect.left;
	Y -= rect.top;
	this->x = rect.left;
	this->y = rect.top;
	this->w = rect.left + rect.right;
	this->h = rect.bottom - rect.top;
	JSON j = JSON::parse(p);
	get(j, "fontName", fontName);
	get(j, "fontSize", fontSize);
	if (text.empty()) get(j, "text", text);
}

void ShadowPainter::draw(Graphics& graphics)
{
	int xx, yy;
	REAL x1, x2, x3, y1, y2, y3;
	Region screen(Rect(0, 0, w, h));
	screen.Exclude(Rect(X, Y, W, H));
	SolidBrush brush(Color(color.GetAlpha(), 0, 0, 0));
	graphics.FillRegion(&brush, &screen);

	int ww = max(X, w - (X + W));
	int hh = max(Y, h - (Y + H));
	int d = 30;

	if (ww * h < hh * w) {
		xx = (2 * X + W > w) ? 0 : w / 2;
		yy = (2 * Y + H > h) ? 0 : Y + H;
		ww = w / 2;
		x1 = (REAL)X + (REAL)W / 2;
		y1 = REAL(yy ? yy + d : hh - d);
		pos = xx ? AP::L : AP::R;
	}
	else {
		xx = (2 * X + W > w) ? 0 : X + W;
		yy = (2 * Y + H > h) ? 0 : h / 2;
		hh = h / 2;
		y1 = (REAL)Y + REAL(H) / 2;
		x1 = REAL(xx ? xx + d : ww - d);
		pos = yy ? AP::T : AP::B;
	}

	SolidBrush textBrush(Color::White);
	FontFamily fontFamily(fontName.c_str());
	Font font(&fontFamily, fontSize, FontStyleRegular, UnitPoint);
	StringFormat format;
	format.SetAlignment(StringAlignment::StringAlignmentCenter);
	format.SetLineAlignment(StringAlignment::StringAlignmentCenter);
	RectF rect((REAL)xx, (REAL)yy, (REAL)ww, (REAL)hh), r;
	graphics.MeasureString((WCHAR*)text.c_str(), (int)text.size(), &font, rect, &format, &r);
	graphics.DrawString((WCHAR*)text.c_str(), (int)text.size(), &font, r, &format, &textBrush);

	switch (pos) {
	case AP::L: x3 = r.X; y3 = r.Y + r.Height / 2; x2 = x1; y2 = y3; break;
	case AP::R: x3 = r.X + r.Width; y3 = r.Y + r.Height / 2; x2 = x1; y2 = y3; break;
	case AP::T: x3 = r.X + r.Width / 2; y3 = r.Y; x2 = x3; y2 = y1; break;
	case AP::B: x3 = r.X + r.Width / 2; y3 = r.Y + r.Height; x2 = x3; y2 = y1; break;
	}

	Pen pen(Color::White, (REAL)thick);
	PointF points[] = { {(REAL)x1, (REAL)y1}, {x2, y2}, {x2, y2}, {x3, y3} };
	AdjustableArrowCap arrow(12, 12, false);
	pen.SetCustomStartCap(&arrow);
	graphics.DrawBeziers(&pen, points, 4);
}

SpeechBubble::SpeechBubble(const std::string& p, int x, int y, int w, int h, const std::wstring& t)
	: PainterBase(p, x, y, w, h), text(t)
{
	this->delay = 0;
	this->x -= thick;
	this->y -= thick;
	this->w += 2 * thick;
	this->h += 2 * thick;
	JSON j = JSON::parse(p);
	get(j, "radius", R);
	get(j, "shape", shape);
	get(j, "color", color);
	get(j, "background", background);
	get(j, "tailWidth", tailWidth);
	get(j, "tailLength", tailLength);
	get(j, "tailRotation", tailRotation);
	get(j, "fontColor", fontColor);
	get(j, "fontName", fontName);
	get(j, "fontSize", fontSize);
	if (text.empty()) get(j, "text", text);
	tailRotation = tailRotation + 180;
	X = Y = tailLength + thick;
	W = w; H = h;
	this->x -= tailLength;
	this->y -= tailLength;
	this->w += 2 * tailLength;
	this->h += 2 * tailLength;
}

void SpeechBubble::draw(Graphics& graphics)
{
	SolidBrush brush(background);
	Pen pen(color, (REAL)thick * 2);
	GraphicsPath path, tail;
	switch (shape) {
	case 0:
		path.AddEllipse(X, Y, W, H);
		break;
	case 1:
		path.AddRectangle(Rect(X, Y, W, H));
		break;
	case 2:
		if (R * 2 > min(W, H)) R = min(W, H) / 2;
		path.AddLine(X + R, Y, X + W - (R * 2), Y);
		path.AddArc(X + W - (R * 2), Y, R * 2, R * 2, 270, 90);
		path.AddLine(X + W, Y + R, X + W, Y + H - (R * 2));
		path.AddArc(X + W - (R * 2), Y + H - (R * 2), R * 2, R * 2, 0, 90);
		path.AddLine(X + W - (R * 2), Y + H, X + R, Y + H);
		path.AddArc(X, Y + H - (R * 2), R * 2, R * 2, 90, 90);
		path.AddLine(X, Y + H - (R * 2), X, Y + R);
		path.AddArc(X, Y, R * 2, R * 2, 180, 90);
		break;
	default:
		path.AddRectangle(Rect(X, Y, W, H));
	}

	graphics.DrawPath(&pen, &path);

	auto gstate = graphics.Save();
	tail.AddLine(-tailWidth, (REAL)0, tailWidth, (REAL)0);
	tail.AddLine(tailWidth, (REAL)0, (REAL)0, tailLength);
	tail.CloseFigure();
	graphics.TranslateTransform((REAL)w / 2, (REAL)h / 2);
	graphics.RotateTransform(tailRotation);
	graphics.DrawPath(&pen, &tail);
	graphics.Restore(gstate);

	gstate = graphics.Save();
	graphics.FillPath(&brush, &path);
	graphics.TranslateTransform((REAL)w / 2, (REAL)h / 2);
	graphics.RotateTransform(tailRotation);
	graphics.FillPath(&brush, &tail);
	graphics.Restore(gstate);

	SolidBrush textBrush(fontColor);
	FontFamily fontFamily(fontName.c_str());
	Font font(&fontFamily, fontSize, FontStyleRegular, UnitPoint);
	StringFormat format;
	format.SetAlignment(StringAlignment::StringAlignmentCenter);
	format.SetLineAlignment(StringAlignment::StringAlignmentCenter);
	RectF rect((REAL)X, (REAL)Y, (REAL)W, (REAL)H), r;
	graphics.MeasureString((WCHAR*)text.c_str(), (int)text.size(), &font, rect, &format, &r);
	graphics.DrawString((WCHAR*)text.c_str(), (int)text.size(), &font, r, &format, &textBrush);
}

SpeechRect::SpeechRect(const std::string& p, int x, int y, const std::wstring& t)
	: PainterBase(p), tx(x), ty(y), text(t)
{
	this->delay = 0;
	JSON j = JSON::parse(p);
	get(j, "radius", R);
	get(j, "color", color);
	get(j, "background", background);
	get(j, "fontColor", fontColor);
	get(j, "fontName", fontName);
	get(j, "fontSize", fontSize);
	if (text.empty()) get(j, "text", text);

	MONITORINFO mi{ 0 };
	mi.cbSize = sizeof(MONITORINFO);
	POINT pt{ x, y };
	auto hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
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
	REAL padding = fontSize + (REAL)thick;
	W = int(r.Width + padding * 2);
	H = int(r.Height + padding * 2);

	int delta = thick * 5;
	int tailLength = int(fontSize * 6);
	if (x * 2 < mr.left + mr.right) {
		dx = delta;
		tw = tailLength;
		X = x;
	}
	else {
		dx = -delta;
		tw = - tailLength;
		X = x - W;
	}
	if (y * 2 < mr.top + mr.bottom) {
		dy = delta;
		th = tailLength;
		Y = y + tailLength;
	}
	else {
		dy = -delta;
		th = - tailLength;
		Y = y - tailLength - H;
	}

	this->x = min(x, X) - delta * 2;
	this->y = min(y, Y) - delta * 2;
	this->w = max(x, X + W) + delta * 4;
	this->h = max(y, Y + H) + delta * 4;
}

void SpeechRect::draw(Graphics& graphics)
{
	auto gstate = graphics.Save();
	graphics.TranslateTransform(dx - x, dy - y);
	SolidBrush brush(background);
	Pen pen(color, (REAL)thick * 2);
	GraphicsPath path, tail;
	if (R < 1) {
		path.AddRectangle(Rect(X, Y, W, H));
	} else {
		if (R * 2 > min(W, H)) R = min(W, H) / 2;
		path.AddLine(X + R, Y, X + W - (R * 2), Y);
		path.AddArc(X + W - (R * 2), Y, R * 2, R * 2, 270, 90);
		path.AddLine(X + W, Y + R, X + W, Y + H - (R * 2));
		path.AddArc(X + W - (R * 2), Y + H - (R * 2), R * 2, R * 2, 0, 90);
		path.AddLine(X + W - (R * 2), Y + H, X + R, Y + H);
		path.AddArc(X, Y + H - (R * 2), R * 2, R * 2, 90, 90);
		path.AddLine(X, Y + H - (R * 2), X, Y + R);
		path.AddArc(X, Y, R * 2, R * 2, 180, 90);
	}
	tail.AddLine((REAL)(tx + tw), (REAL)(ty + th * 1.33), (REAL)(tx), (REAL)(ty));
	tail.AddLine((REAL)(tx), (REAL)(ty), (REAL)(tx + tw * 1.33), (REAL)(ty + th));
	tail.CloseFigure();

	graphics.DrawPath(&pen, &path);
	graphics.DrawPath(&pen, &tail);
	graphics.FillPath(&brush, &path);
	graphics.FillPath(&brush, &tail);

	SolidBrush textBrush(fontColor);
	FontFamily fontFamily(fontName.c_str());
	Font font(&fontFamily, fontSize, FontStyleRegular, UnitPoint);
	StringFormat format;
	format.SetAlignment(StringAlignment::StringAlignmentCenter);
	format.SetLineAlignment(StringAlignment::StringAlignmentCenter);
	RectF rect((REAL)X, (REAL)Y, (REAL)W, (REAL)H), r;
	graphics.MeasureString((WCHAR*)text.c_str(), (int)text.size(), &font, rect, &format, &r);
	graphics.DrawString((WCHAR*)text.c_str(), (int)text.size(), &font, r, &format, &textBrush);
	graphics.Restore(gstate);
}

void EllipsePainter::draw(Graphics& graphics)
{
	Pen pen(color, (REAL)thick);
	graphics.DrawEllipse(&pen, thick, thick, w - 2 * thick, h - 2 * thick);
}

BezierPainter::BezierPainter(const std::string& params, const std::string& text)
	: PainterBase(params)
{
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
}

void BezierPainter::draw(Graphics& graphics)
{
	REAL z = (REAL)thick;
	Pen pen(color, z);
	AdjustableArrowCap arrow(8, 4);
	pen.SetStartCap(LineCapRoundAnchor);
	pen.SetCustomEndCap(&arrow);
	auto p = points;
	int X = p[0].X;
	int Y = p[0].Y;
	for (auto it = p.begin() + 1; it != p.end(); ++it) {
		it->X = X + (it->X - X) * step / limit;
		it->Y = Y + (it->Y - Y) * step / limit;
	}
	graphics.DrawBeziers(&pen, p.data(), (INT)p.size());
}

void ArrowPainter::draw(Graphics& graphics)
{
	REAL z = (REAL)thick;
	Pen pen(color, z);
	AdjustableArrowCap arrow(8, 4);
	pen.SetStartCap(LineCapRoundAnchor);
	pen.SetCustomEndCap(&arrow);
	int X1 = x1 - x;
	int Y1 = y1 - y;
	int X2 = X1 + (x2 - x1) * step / limit;
	int Y2 = Y1 + (y2 - y1) * step / limit;
	graphics.DrawLine(&pen, X1, Y1, X2, Y2);
}

LRESULT PainterBase::repaint(HWND hWnd)
{
	GgiPlusToken::Init();
	Bitmap bitmap(w, h, PixelFormat32bppARGB);
	Graphics graphics(&bitmap);
	graphics.Clear(Color::Transparent);
	graphics.SetCompositingQuality(CompositingQuality::CompositingQualityHighQuality);
	graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	graphics.SetTextRenderingHint(TextRenderingHint::TextRenderingHintAntiAlias);
	draw(graphics);

	if (delay) {
		if (step >= limit)
			KillTimer(hWnd, ID_TIMER_REPAINT);
		else ++step;
	}

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
		return ((PainterBase*)((CREATESTRUCT*)lParam)->lpCreateParams)->repaint(hWnd);
	case WM_TIMER:
		switch (wParam) {
		case ID_TIMER_REPAINT:
			((PainterBase*)GetWindowLongPtr(hWnd, GWLP_USERDATA))->repaint(hWnd);
			break;
		case ID_TIMER_TIMEOUT:
			KillTimer(hWnd, ID_TIMER_REPAINT);
			KillTimer(hWnd, ID_TIMER_TIMEOUT);
			SendMessage(hWnd, WM_DESTROY, 0, 0);
			break;
		}
		return 0;
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
	WNDCLASS wndClass = {};
	wndClass.lpfnWndProc = PainterWndProc;
	wndClass.hInstance = hModule;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszClassName = name;
	RegisterClass(&wndClass);

	DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;
	HWND hWnd = CreateWindowEx(dwExStyle, name, name, WS_POPUP, x, y, w, h, NULL, NULL, hModule, this);
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
	if (delay) SetTimer(hWnd, ID_TIMER_REPAINT, delay, NULL);
	SetTimer(hWnd, ID_TIMER_TIMEOUT, duration, NULL);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);
}

static DWORD WINAPI PainterThreadProc(LPVOID lpParam)
{
	std::unique_ptr<PainterBase> painter((PainterBase*)lpParam);
	painter->create();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

void PainterBase::run()
{
	CreateThread(0, NULL, PainterThreadProc, (LPVOID)this, NULL, NULL);
}

#endif //_WINDOWS