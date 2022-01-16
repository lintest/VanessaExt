#pragma once

#ifdef _WINDOWS

#include "ImageHelper.h"

using namespace Gdiplus;

class EducationShow {
private:
	std::wstring title, button, identifier;
	std::wstring eventName = L"STOP_VANESSA";
	std::wstring eventData;
	int titleWidth = 0;
	int buttonWidth = 0;
	int buttonHeight = 0;
	int buttonIconWidth = 0;
	int buttonTextWidth = 0;
	Color color = { 200, 50, 50 };
	int x = 0, y = 0, w = 0, h = 0;
	int trans = 127;
	int thick = 4;
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
public:
	EducationShow(AddInNative& addin, const std::string& p, const std::wstring& title, const std::wstring& button);
	LRESULT onHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseDown(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseMove(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseHover(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onMouseLeave(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT repaint(HWND hWnd);
	static void close();
	void create();
	void run();
};

#endif //_WINDOWS
