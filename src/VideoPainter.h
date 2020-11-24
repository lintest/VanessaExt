#ifndef VIDEOPAINTER_H
#define VIDEOPAINTER_H

#ifdef _WINDOWS
#include "windows.h"

class PainterBase {
protected:
	int x = 0, y = 0, w = 0, h = 0;
	COLORREF m_color = RGB(200, 50, 50);
	int m_delay = 2000;
	int m_trans = 127;
	int m_thick = 2;
public:
	PainterBase() {}
	PainterBase(int color, int delay, int thick, int trans)
		: m_color((COLORREF)color), m_delay(delay), m_thick(thick), m_trans(trans) {}
	PainterBase(const PainterBase& p)
		: m_color(p.m_color)
		, m_delay(p.m_delay)
		, m_trans(p.m_trans)
		, m_thick(p.m_thick)
	{}
	void init(int color, int delay, int thick, int trans) {
		m_color = color;
		m_delay = delay;
		m_thick = thick;
		m_trans = trans;
	}
	virtual LRESULT paint(HWND hWnd) = 0;
	void create();
};

class VideoPainter
	: public PainterBase {
public:
	static PainterBase* painter(HWND hWnd);
	static PainterBase* painter(LPVOID lpParam);
	virtual LRESULT paint(HWND hWnd) override { return 0; };
public:
	VideoPainter() {}
	VideoPainter(int color, int delay, int thick, int trans)
		: PainterBase(color,  delay, thick, trans) {}
	void ellipse(int left, int top, int width, int height);
	void rect(int left, int top, int width, int height);
};

#endif //_WINDOWS

#endif //VIDEOPAINTER_H