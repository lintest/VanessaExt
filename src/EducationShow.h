#pragma once

#ifdef _WINDOWS

#include "ImageHelper.h"

using namespace Gdiplus;

class EducationShow {
protected:
	std::wstring title, button;
	int titleWidth = 0;
	int buttonWidth = 0;
	Color color = { 200, 50, 50 };
	int x = 0, y = 0, w = 0, h = 0;
	int duration = 5000;
	int trans = 127;
	int delay = 0;
	int thick = 4;
	int limit = 50;
	int step = 10;
	int padding = 4;
	REAL fontSize = 12;
	Color background = Color::White;
	Color fontColor = { 200, 50, 50 };
	Color buttonColor = { 100, 25, 25 };
	std::wstring fontName = L"Calibri";
	bool moving = false;
	bool hover = false;
	int mx = 0;
	int my = 0;
public:
	EducationShow(const std::string& p, const std::wstring& title, const std::wstring& button);
	void draw(Gdiplus::Graphics& graphics, bool hover);
	LRESULT repaint(HWND hWnd, bool hover);
	LRESULT onHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseDown(HWND hWnd, LPARAM lParam);
	LRESULT onMouseMove(HWND hWnd, LPARAM lParam);
	LRESULT onMouseUp(HWND hWnd, LPARAM lParam);
	void create();
	void run();
};

#endif //_WINDOWS
