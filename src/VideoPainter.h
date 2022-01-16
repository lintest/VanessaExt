#pragma once

#ifdef _WINDOWS

#include "ImageHelper.h"

using namespace Gdiplus;

class PainterBase {
protected:
	Color color = { 200, 50, 50 };
	int x = 0, y = 0, w = 0, h = 0;
	int duration = 5000;
	int delay = 0;
	int thick = 4;
	int limit = 50;
	int step = 10;
public:
	PainterBase(const std::string& p, int x = 0, int y = 0, int w = 0, int h = 0);
	virtual ~PainterBase() {}
	virtual LRESULT repaint(HWND hWnd);
	virtual void draw(HWND hWnd, Graphics& graphics) { }
	virtual void init(HWND hWnd) { }
	virtual void onClick() { }
	virtual void create();
	void run();
};

class RecanglePainter
	: public PainterBase {
public:
	RecanglePainter(const std::string& p, int x, int y, int w, int h)
		: PainterBase(p, x, y, w, h) 
	{
		delay = 0;
	}
	virtual void draw(HWND hWnd, Graphics& graphics) override;
};

class EllipsePainter
	: public PainterBase {
public:
	EllipsePainter(const std::string& p, int x, int y, int w, int h)
		: PainterBase(p, x, y, w, h) 
	{
		this->delay = 0;
		this->x -= thick;
		this->y -= thick;
		this->w += 2 * thick;
		this->h += 2 * thick;
	}
	virtual void draw(HWND hWnd, Graphics& graphics) override;
};

class BezierPainter
	: public PainterBase {
private:
	std::vector<Point> points;
public:
	BezierPainter(const std::string& params, const std::string& points);
	virtual void draw(HWND hWnd, Graphics& graphics) override;
};

class ArrowPainter
	: public PainterBase {
private:
	int x1, y1, x2, y2;
public:
	ArrowPainter(const std::string& p, int x1, int y1, int x2, int y2)
		: PainterBase(p), x1(x1), y1(y1), x2(x2), y2(y2)
	{
		x = min(x1, x2) - 2 * thick;
		y = min(y1, y2) - 2 * thick;
		w = abs(x1 - x2) + 4 * thick;
		h = abs(y1 - y2) + 4 * thick;
	}
	virtual void draw(HWND hWnd, Graphics& graphics) override;
};

class ShadowPainter;

class ShadowButton {
private:
	std::wstring title;
	std::wstring eventName = L"SHADOW_BUTTON";
	std::wstring eventData;
	Color borderColor = { 200, 50, 50 };
	int x = 0, y = 0, w = 0, h = 0;
	int trans = 191;
	int thick = 1;
	int margin = 4;
	int padding = 4;
	REAL fontSize = 12;
	Color backColor = Color::White;
	Color fontColor = { 200, 50, 50 };
	std::wstring fontName = L"Calibri";
	bool pressed = false;
	bool hover = false;
	int mx = 0;
	int my = 0;
private:
	HWND m_hWnd = NULL;
	ShadowPainter& m_owner;
	void draw(Gdiplus::Graphics& graphics);
public:
	static LRESULT process(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	ShadowButton(ShadowPainter& owner, const JSON& json, const JSON& j);
	~ShadowButton();
	RectF calculate(Graphics& graphics, const RectF& rect);
	void resize(REAL x, REAL y, REAL w, REAL h);
	void onMouseDown(HWND hWnd);
	void onMouseMove(HWND hWnd);
	void onMouseUp(HWND hWnd);
	void onMouseHover(HWND hWnd);
	void onMouseLeave(HWND hWnd);
	LRESULT repaint(HWND hWnd);
	void create(HWND hParent);
};

class ShadowPainter
	: public PainterBase {
private:
	enum class AP { L, R, T, B };
	int X, Y, W, H;
	REAL fontSize = 24;
	std::wstring eventName = L"SHADOW_EFFECT";
	std::wstring eventData;
	std::wstring fontName = L"Calibri";
	std::wstring text;
	AP pos = AP::T;
private:
	AddInNative& addin;
	HWND m_hWnd = NULL;
	std::vector<std::unique_ptr<ShadowButton>> buttons;
public:
	ShadowPainter(AddInNative& addin, const std::string& p, int x, int y, int w, int h, const std::wstring& text);
	virtual void draw(HWND hWnd, Graphics& graphics) override;
	virtual void create() override;
	virtual void onClick() override;
	void hide(const std::wstring eventName, const std::wstring eventData);
};

class SpeechBubble
	: public PainterBase {
private:
	int shape = 2;
	int X, Y, W, H, R = 20;
	REAL tailWidth = 24;
	REAL tailLength = 100;
	REAL tailRotation = -135;
	REAL fontSize = 12;
	Color background = Color::White;
	Color fontColor = { 200, 50, 50 };
	std::wstring fontName = L"Calibri";
	std::wstring text;
public:
	SpeechBubble(const std::string& p, int x, int y, int w, int h, const std::wstring& text);
	virtual void draw(HWND hWnd, Graphics& graphics) override;
};

class SpeechRect
	: public PainterBase {
private:
	int X, Y, W, H, R = 20;
	int tx, ty, tw, th, dx, dy;
	REAL fontSize = 12;
	Color background = Color::White;
	Color fontColor = { 200, 50, 50 };
	std::wstring fontName = L"Calibri";
	std::wstring text;
public:
	SpeechRect(const std::string& p, int x, int y, const std::wstring& text);
	virtual void draw(HWND hWnd, Graphics& graphics) override;
};

class TextLabel
	: public PainterBase {
private:
	REAL fontSize = 12;
	Color fontColor = { 200, 50, 50 };
	std::wstring fontName = L"Calibri";
	std::wstring text;
public:
	TextLabel(const std::string& p, int x, int y, const std::wstring& text);
	virtual void draw(HWND hWnd, Graphics& graphics) override;
};

#endif //_WINDOWS
