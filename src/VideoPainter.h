#ifndef VIDEOPAINTER_H
#define VIDEOPAINTER_H

#ifdef _WINDOWS
#include "windows.h"
#include "Gdiplus.h"
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

	virtual LRESULT paint(HWND hWnd) = 0;
	void create();
	void start();
};

class VideoPainter
	: public PainterBase {
public:
	static PainterBase* painter(HWND hWnd);
	virtual LRESULT paint(HWND hWnd) override { return 0; };
public:
	void init(int color, int delay, int thick, int trans) {
		color = color;
		delay = delay;
		thick = thick;
		trans = trans;
	}
};

class RecanglePainter
	: public PainterBase {
public:
	RecanglePainter(const VideoPainter& p, int x, int y, int w, int h)
		: PainterBase(p, x, y, w, h) {
		start();
	}
	virtual LRESULT paint(HWND hWnd) override;
};

class EllipsePainter
	: public PainterBase {
public:
	EllipsePainter(const VideoPainter& p, int x, int y, int w, int h)
		: PainterBase(p, x, y, w, h) 
	{
		GgiPlusToken::Init();
		start();
	}
	virtual LRESULT paint(HWND hWnd) override;
};

class PolyBezierPainter
	: public PainterBase {
private:
	std::vector<Gdiplus::Point> points;
public:
	PolyBezierPainter(const VideoPainter& p, const std::string& points);
	virtual LRESULT paint(HWND hWnd) override;
};

#endif //_WINDOWS

#endif //VIDEOPAINTER_H