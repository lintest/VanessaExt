#pragma once

#ifdef _WINDOWS

#include "ImageHelper.h"

using namespace Gdiplus;

class EducationShow {
protected:
	std::wstring text;
	Color color = { 200, 50, 50 };
	int x = 0, y = 0, w = 0, h = 0;
	int duration = 5000;
	int trans = 127;
	int delay = 0;
	int thick = 4;
	int limit = 50;
	int step = 10;
	REAL fontSize = 12;
	Color background = Color::White;
	Color fontColor = { 200, 50, 50 };
	std::wstring fontName = L"Calibri";
public:
	EducationShow(const std::string& p, int x, int y, const std::wstring& t);
	void draw(Gdiplus::Graphics& graphics);
	LRESULT repaint(HWND hWnd);
	void create();
	void run();
};

#endif //_WINDOWS
