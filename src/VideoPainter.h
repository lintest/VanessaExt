#ifndef VIDEOPAINTER_H
#define VIDEOPAINTER_H

#ifdef _WINDOWS

#include "ImageHelper.h"

class PainterBase {
protected:
	int x = 0, y = 0, w = 0, h = 0;
	COLORREF color = RGB(200, 50, 50);
	int delay = 5000;
	int trans = 255;
	int thick = 4;
public:
	PainterBase() {}

	PainterBase(int color, int delay, int thick, int trans)
		: color((COLORREF)color), delay(delay), trans(trans), thick(thick) {}

	PainterBase(const PainterBase& p, int x, int y, int w, int h)
		: color(p.color), delay(p.delay), trans(p.trans), thick(p.thick), x(x), y(y), w(w), h(h) {}

	PainterBase(const PainterBase& p)
		: color(p.color), delay(p.delay), trans(p.trans), thick(p.thick) {}

	virtual ~PainterBase() {}
	LRESULT create(HWND hWnd);
	void createThread();
	void createWindow();

	virtual void paint(Gdiplus::Graphics& graphics) { };
};

class VideoPainter
	: public PainterBase {
public:
	void init(int color, int delay, int thick, int trans) {
		this->color = color;
		this->delay = delay;
		this->thick = thick;
		this->trans = trans;
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
	virtual void paint(Gdiplus::Graphics& graphics) override;
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
	virtual void paint(Gdiplus::Graphics& graphics) override;
};

class PolyBezierPainter
	: public PainterBase {
private:
	std::vector<Gdiplus::Point> points;
public:
	PolyBezierPainter(const VideoPainter& p, const std::string& points);
	virtual void paint(Gdiplus::Graphics& graphics) override;
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
	virtual void paint(Gdiplus::Graphics& graphics) override;
};

#endif //_WINDOWS

#endif //VIDEOPAINTER_H