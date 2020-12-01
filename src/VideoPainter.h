#ifndef VIDEOPAINTER_H
#define VIDEOPAINTER_H

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
	virtual void draw(Graphics& graphics) { };
	void create();
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
	virtual void draw(Graphics& graphics) override;
};

class ShadowPainter
	: public PainterBase {
private:
	int X, Y, W, H;
	int transparency;
public:
	ShadowPainter(const std::string& p, int x, int y, int w, int h, int t)
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
	}
	virtual void draw(Graphics& graphics) override;
};

class EllipsePainter
	: public PainterBase {
public:
	EllipsePainter(const std::string& p, int x, int y, int w, int h)
		: PainterBase(p, x, y, w, h) 
	{
		delay = 0;
		x -= thick;
		y -= thick;
		w += 2 * thick;
		h += 2 * thick;
	}
	virtual void draw(Graphics& graphics) override;
};

class BezierPainter
	: public PainterBase {
private:
	std::vector<Point> points;
public:
	BezierPainter(const std::string& params, const std::string& points);
	virtual void draw(Graphics& graphics) override;
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
	virtual void draw(Graphics& graphics) override;
};

#endif //_WINDOWS

#endif //VIDEOPAINTER_H
