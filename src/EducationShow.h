#pragma once

#ifdef _WINDOWS

#include "ImageHelper.h"

using namespace Gdiplus;

class EducationShow {
private:
	std::wstring title, button, identifier;
	int titleWidth = 0;
	int buttonWidth = 0;
	int buttonHeight = 0;
	int buttonIconWidth = 0;
	int buttonTextWidth = 0;
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
	Color iconColor = Color::Red;
	Color stopColor = Color::White;
	std::wstring fontName = L"Calibri";
	bool pressed = false;
	bool hover = false;
	int mx = 0;
	int my = 0;
	AddInNative& addin;
private:
	void draw(Gdiplus::Graphics& graphics);
	LRESULT repaint(HWND hWnd);
public:
	EducationShow(AddInNative& addin, const std::string& p, const std::wstring& title, const std::wstring& button);
	LRESULT onHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseDown(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseMove(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseHover(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseLeave(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void close();
	void create();
	void run();
};

#endif //_WINDOWS
