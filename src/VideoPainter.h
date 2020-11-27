#ifndef VIDEOPAINTER_H
#define VIDEOPAINTER_H

#ifdef _WINDOWS

#include "ImageHelper.h"

using namespace Gdiplus;

class PainterBase {
protected:
	Color color = { 200, 50, 50 };
	int x = 0, y = 0, w = 0, h = 0;
	int delay = 5000;
	int thick = 4;
public:
	PainterBase() {}

	PainterBase(const PainterBase& p, int x, int y, int w, int h)
		: color(p.color), delay(p.delay), thick(p.thick), x(x), y(y), w(w), h(h) {}

	PainterBase(const PainterBase& p)
		: color(p.color), delay(p.delay), thick(p.thick) {}

	virtual ~PainterBase() {}
	virtual LRESULT create(HWND hWnd);
	virtual void createThread();
	virtual void createWindow();
	virtual void draw(Graphics& graphics) { };
};

class VideoPainter
	: public PainterBase {
public:
	void init(int color, int delay, int thick, int trans) {
		Color c;
		c.SetFromCOLORREF(color);
		this->color = Color(trans & 0xFF, c.GetRed(), c.GetGreen(), c.GetBlue());
		this->delay = delay;
		this->thick = thick;
	}
};

class RecanglePainter
	: public PainterBase {
public:
	RecanglePainter(const VideoPainter& p, int x, int y, int w, int h)
		: PainterBase(p, x, y, w, h)
	{
		createThread();
	}
	virtual void draw(Graphics& graphics) override;
};

class ShadowPainter
	: public PainterBase {
private:
	int X, Y, W, H;
	int transparency;
public:
	ShadowPainter(const VideoPainter& p, int x, int y, int w, int h, int t)
		: PainterBase(p), X(x), Y(y), W(w), H(h), transparency(t)
	{
		RECT rect;
		GetWindowRect(GetDesktopWindow(), &rect);
		X -= rect.left; 
		Y -= rect.top;
		this->x = rect.left;
		this->y = rect.top;
		this->w = rect.left + rect.right;
		this->h = rect.bottom - rect.top;
		createThread();
	}
	virtual void draw(Graphics& graphics) override;
};

class EllipsePainter
	: public PainterBase {
public:
	EllipsePainter(const VideoPainter& p, int x, int y, int w, int h)
		: PainterBase(p, x, y, w, h) 
	{
		x -= thick;
		y -= thick;
		w += 2 * thick;
		h += 2 * thick;
		createThread();
	}
	virtual void draw(Graphics& graphics) override;
};

class BezierPainter
	: public PainterBase {
private:
	std::vector<Point> points;
public:
	BezierPainter(const VideoPainter& p, const std::string& points);
	virtual void draw(Graphics& graphics) override;
};

class ArrowPainter
	: public PainterBase {
private:
	int x1, y1, x2, y2;
public:
	ArrowPainter(const VideoPainter& p, int x1, int y1, int x2, int y2)
		: PainterBase(p), x1(x1), y1(y1), x2(x2), y2(y2)
	{
		x = min(x1, x2) - 2 * thick;
		y = min(y1, y2) - 2 * thick;
		w = abs(x1 - x2) + 4 * thick;
		h = abs(y1 - y2) + 4 * thick;
		createThread();
	}
	virtual void draw(Graphics& graphics) override;
};

#endif //_WINDOWS

#endif //VIDEOPAINTER_H
