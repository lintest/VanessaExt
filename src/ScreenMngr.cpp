#include "stdafx.h"
#include "ScreenMngr.h"
#include "json_ext.h"

#ifdef __linux__

#include <fstream>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#define p_verbose(...) if (envir_verbose) { \
    fprintf(stderr, __VA_ARGS__); \
}

static bool envir_verbose = false;

using namespace std;

#define VXX(T) reinterpret_cast<void**>(T)

class WindowHelper
{
protected:
    JSON json;
    Display* display = NULL;
    unsigned long count = 0;

public:
    WindowHelper() {
        display = XOpenDisplay(NULL);
	}
    ~WindowHelper() {
        if (display) XCloseDisplay(display);
    }

	operator std::wstring() {
		return json;
	}

protected:
    bool GetProperty(Window window, Atom xa_prop_type, const char *prop_name, void **ret_prop, unsigned long *size) {
        int ret_format;
        Atom xa_prop_name, xa_ret_type;
        unsigned long ret_bytes_after;
        xa_prop_name = XInternAtom(display, prop_name, False);
        if (XGetWindowProperty(display, window, xa_prop_name, 0, ~0L, False,
                xa_prop_type, &xa_ret_type, &ret_format,
                size, &ret_bytes_after, (unsigned char**)ret_prop) != Success) {
            p_verbose("Cannot get %s property.\n", prop_name);
            return false;
        }
        if (xa_ret_type != xa_prop_type) {
            p_verbose("Invalid type of %s property.\n", prop_name);
            XFree(*ret_prop);
            return false;
        }
        return true;
    }

    int SendMessage(Window window, char *msg, 
        unsigned long data0 = 0, unsigned long data1 = 0, 
        unsigned long data2 = 0, unsigned long data3 = 0,
        unsigned long data4 = 0) 
    {
        XEvent event;
        long mask = SubstructureRedirectMask | SubstructureNotifyMask;

        event.xclient.type = ClientMessage;
        event.xclient.serial = 0;
        event.xclient.send_event = True;
        event.xclient.message_type = XInternAtom(display, msg, False);
        event.xclient.window = window;
        event.xclient.format = 32;
        event.xclient.data.l[0] = data0;
        event.xclient.data.l[1] = data1;
        event.xclient.data.l[2] = data2;
        event.xclient.data.l[3] = data3;
        event.xclient.data.l[4] = data4;

        if (XSendEvent(display, DefaultRootWindow(display), False, mask, &event)) {
            return EXIT_SUCCESS;
        }
        else {
            fprintf(stderr, "Cannot send %s event.\n", msg);
            return EXIT_FAILURE;
        }
    }

    bool GetWindowTitle(Window window, char **title) {
        unsigned long size;
        if (GetProperty(window, XInternAtom(display, "UTF8_STRING", False), "_NET_WM_NAME", VXX(title), &size)) return true;
        if (GetProperty(window, XA_STRING, "WM_NAME", VXX(title), NULL)) return true;
        return false;
    }

    bool GetWindowClass(Window window, char **title) {
        unsigned long size;
        if (!GetProperty(window, XA_STRING, "WM_CLASS", VXX(title), &size)) return false;
        char *p0 = strchr(*title, '\0');
        if (*title + size - 1 > p0) *(p0) = '.';
        return true;
    }

public:
    Window GetActiveWindow() {
        Window *buffer = NULL;
        unsigned long size;
        Window root = DefaultRootWindow(display);
        if (!GetProperty(root, XA_WINDOW, "_NET_ACTIVE_WINDOW", VXX(&buffer), &size)) return NULL;
        Window result = buffer ? *buffer : NULL;
        XFree(buffer);
        return result;
    }

    void SetActiveWindow(Window window) {
        SendMessage(window, "_NET_ACTIVE_WINDOW", 1, CurrentTime);
    }

     void Maximize(Window window, bool state) {
        SendMessage(window, "_NET_WM_STATE_HIDDEN", 0, CurrentTime);
        Atom prop1 = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        Atom prop2 = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
        SendMessage(window, "_NET_WM_STATE", state ? 1 : 2, prop1, prop2);
    }

    void Minimize(Window window) {
        SendMessage(window, "_NET_WM_STATE_HIDDEN", 1, CurrentTime);
		XLowerWindow(display, window);
    }

	void SetWindowPos(Window window, unsigned long x, unsigned long y) {
		XMoveWindow(display, window, x, y);
	}

	void SetWindowSize(Window window, unsigned long w, unsigned long h) {
		XResizeWindow(display, window, w, h);
	}
};

class WindowList : public WindowHelper
{
protected:
    Window* windows = NULL;

public	:
    WindowList() {
        Window root = DefaultRootWindow(display);
        if (!GetProperty(root, XA_WINDOW, "_NET_CLIENT_LIST", VXX(&windows), &count)) return;
        for (int i = 0; i < count; i++) {
            JSON j;
            Window window = windows[i];

            unsigned long size;
            unsigned long *pid = NULL;
            GetProperty(window, XA_CARDINAL, "_NET_WM_PID", VXX(&pid), &size);
            j["pid"] = *pid;
            j["window"] = (unsigned long)window;
            XFree(pid);

            /* geometry */
            Window junkroot;
            int x, y, junkx, junky;
            unsigned int w, h, bw, depth;
            XGetGeometry(display, window, &junkroot, &junkx, &junky, &w, &h, &bw, &depth);
            XTranslateCoordinates(display, window, junkroot, junkx, junky, &x, &y, &junkroot);

            char *name = NULL;
            char *title = NULL;
            GetWindowClass(window, &name);
            GetWindowTitle(window, &title);
            j["class"] = name;
            j["title"] = title;
            XFree(title);
            XFree(name);
            json.push_back(j);
        }
    }
	~WindowList() {
        XFree(windows);
	}
};

class GeometryHelper : public WindowHelper 
{
public:
    GeometryHelper(Window window) {
        /* geometry */
        Window junkroot;
        int x, y, junkx, junky;
        unsigned int w, h, bw, depth;
        Status status = XGetGeometry(display, window, &junkroot, &junkx, &junky, &w, &h, &bw, &depth);
        if (!status) return;
        Bool ok = XTranslateCoordinates(display, window, junkroot, junkx, junky, &x, &y, &junkroot);
        if (!ok) return;
        json["left"] = x;
        json["top"] = y;
        json["width"] = w;
        json["height"] = h;
        json["right"] = x + w;
        json["bottom"] = y + h;
    }
};

std::wstring ScreenManager::GetWindowList(tVariant* paParams, const long lSizeArray)
{
	return WindowList();
}

std::wstring ScreenManager::GetDisplayInfo(tVariant* paParams, const long lSizeArray)
{
	JSON json;
	Display* display = XOpenDisplay(NULL);
	if (!display) return {};
	int number = DefaultScreen(display);
	json["left"] = json["top"] = 0;
	json["right"] = json["width"] = DisplayWidth(display, number);
	json["bottom"] = json["height"] = DisplayHeight(display, number);
	XCloseDisplay(display);
	return WSTR(json);
}

HWND ScreenManager::ActiveWindow()
{
	return WindowHelper().GetActiveWindow();
}

BOOL ScreenManager::Activate(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return false;
	Window window =  VarToInt(paParams);
	WindowHelper().SetActiveWindow(window);
	return true;
}

BOOL  ScreenManager::Restore(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return false;
	Window window =  VarToInt(paParams);
	WindowHelper().Maximize(window, false);
	return true;
}

BOOL  ScreenManager::Maximize(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return false;
	Window window =  VarToInt(paParams);
	WindowHelper().Maximize(window, true);
	return true;
}

BOOL ScreenManager::Minimize(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return false;
	Window window =  VarToInt(paParams);
	WindowHelper().Minimize(window);
	return true;
}

BOOL ScreenManager::SetWindowSize(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 3) return false;
	Window window =  VarToInt(paParams);
	int x = VarToInt(paParams + 1);
	int y = VarToInt(paParams + 2);
	WindowHelper().SetWindowSize(window, x, y);
	return true;
}

BOOL ScreenManager::SetWindowPos(tVariant* paParams, const long lSizeArray) 
{
	if (lSizeArray < 3) return false;
	Window window =  VarToInt(paParams);
	int x = VarToInt(paParams + 1);
	int y = VarToInt(paParams + 2);
	WindowHelper().SetWindowPos(window, x, y);
	return true;
}

#else//__linux__

static JSON RectToJson(const RECT& rect)
{
	JSON json;
	json["left"] = rect.left;
	json["top"] = rect.top;
	json["right"] = rect.right;
	json["bottom"] = rect.bottom;
	json["width"] = rect.right - rect.left;
	json["height"] = rect.bottom - rect.top;
	return json;
}

std::wstring ScreenManager::GetDisplayList(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);

	HDC hdc = NULL;
	RECT rect;
	LPCRECT lpRect = NULL;
	if (hWnd) {
		GetClientRect(hWnd, &rect);
		hdc = GetDC(hWnd);
		lpRect = &rect;
	}

	JSON json;
	BOOL bResult = ::EnumDisplayMonitors(hdc, lpRect, [](HMONITOR hMonitor, HDC, LPRECT rect, LPARAM lParam) -> BOOL
		{
			JSON j = RectToJson(*rect);
			MONITORINFOEX mi;
			mi.cbSize = sizeof(mi);
			if (::GetMonitorInfo(hMonitor, &mi)) {
				j["name"] = WC2MB((mi.szDevice));
				j["work"] = RectToJson(mi.rcWork);
			}
			JSON* json = (JSON*)lParam;
			json->push_back(j);
			return TRUE;
		}, (LPARAM)&json);

	if (hdc) ReleaseDC(hWnd, hdc);

	return json;
}

std::wstring ScreenManager::GetDisplayInfo(tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
	if (!hWnd) hWnd = ::GetForegroundWindow();
	if (!hWnd) return {};

	RECT rect;
	GetWindowRect(hWnd, &rect);
	HMONITOR hMonitor = ::MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	if (!hMonitor) return {};

	MONITORINFOEX mi;
	mi.cbSize = sizeof(mi);
	BOOL bResult = ::GetMonitorInfo(hMonitor, &mi);
	if (!bResult) return {};

	JSON json = RectToJson(mi.rcMonitor);
	json["name"] = WC2MB((mi.szDevice));
	json["work"] = RectToJson(mi.rcWork);
	return json;
}

std::wstring ScreenManager::GetScreenInfo()
{
	RECT rect, work{0,0,0,0};
	rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
	rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
	JSON json = RectToJson(rect);
	json["work"] = RectToJson(work);
	return json;
}

#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

BOOL ScreenManager::CaptureScreen(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	const int mode = lSizeArray == 0 ? 0 : VarToInt(paParams);
	if (mode == 2) return CaptureWindow(pvarRetValue, 0);

	HWND hWnd = ::GetForegroundWindow();
	UpdateWindow(hWnd);

	LONG x, y, w, h;
	if (mode == 1) {
		RECT rect;
		DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
		x = rect.left;
		y = rect.top;
		w = rect.right - rect.left;
		h = rect.bottom - rect.top;
	}
	else {
		x = 0;
		y = 0;
		w = GetSystemMetrics(SM_CXSCREEN);
		h = GetSystemMetrics(SM_CYSCREEN);
	}

	HDC hScreen = GetDC(NULL);
	HDC hDC = CreateCompatibleDC(hScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
	HGDIOBJ object = SelectObject(hDC, hBitmap);
	BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);
	const BOOL result = SaveBitmap(hBitmap, pvarRetValue);

	SelectObject(hDC, object);
	DeleteDC(hDC);
	ReleaseDC(NULL, hScreen);
	DeleteObject(hBitmap);

	return result;
}

BOOL ScreenManager::CaptureWindow(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = VarToHwnd(paParams);
	return CaptureWindow(pvarRetValue, hWnd);
}

BOOL ScreenManager::CaptureWindow(tVariant* pvarRetValue, HWND hWnd)
{
	if (hWnd == 0) hWnd = ::GetForegroundWindow();

	RECT rc;
	GetClientRect(hWnd, &rc);
	DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rc, sizeof(rc));

	HDC hdcScreen = GetDC(NULL);
	HDC hDC = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, rc.right - rc.left, rc.bottom - rc.top);
	SelectObject(hDC, hBitmap);
	PrintWindow(hWnd, hDC, 0);

	const BOOL result = SaveBitmap(hBitmap, pvarRetValue);

	ReleaseDC(NULL, hdcScreen);
	DeleteDC(hDC);
	DeleteObject(hBitmap);

	return result;
}

#include <gdiplus.h>
#pragma comment(lib, "Gdiplus.lib")

class GgiPlusToken {
private:
	ULONG_PTR h = NULL;
public:
	GgiPlusToken() {}
	~GgiPlusToken() { if (h) Gdiplus::GdiplusShutdown(h); }
	ULONG_PTR* operator &() { return &h; }
	BOOL operator!() { return !h; }
};

static GgiPlusToken gdiplusToken;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0) return -1;  // Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL) return -1;  // Failure

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

BOOL ScreenManager::SaveBitmap(HBITMAP hBitmap, tVariant* pvarRetValue)
{
	BOOL Ret = FALSE;
	Gdiplus::Status status = Gdiplus::Ok;
	if (!gdiplusToken) // initialization of gdi+
	{
		const Gdiplus::GdiplusStartupInput input;
		status = Gdiplus::GdiplusStartup(&gdiplusToken, &input, NULL);
		if (status != Gdiplus::Ok) return false;
	}

	CLSID clsid;
	GetEncoderClsid(L"image/png", &clsid); // retrieving JPEG encoder CLSID

	Gdiplus::Bitmap SrcBitmap(hBitmap, 0); // creating bitmap

	IStream* pStream;
	if (SUCCEEDED(CreateStreamOnHGlobal(0, TRUE, &pStream))) // creating stream
	{
		status = SrcBitmap.Save(pStream, &clsid, 0); // saving image to the stream
		if (status == Gdiplus::Ok)
		{
			LARGE_INTEGER lOfs;
			ULARGE_INTEGER lSize;
			lOfs.QuadPart = 0;
			if (SUCCEEDED(pStream->Seek(lOfs, STREAM_SEEK_END, &lSize))) // retrieving size of stream data (seek to end)
			{
				lOfs.QuadPart = 0;
				if (SUCCEEDED(pStream->Seek(lOfs, STREAM_SEEK_SET, 0))) // seeking to beginning of the stream data
				{
					pvarRetValue->strLen = (ULONG)((DWORD_PTR)lSize.QuadPart);
					m_iMemory->AllocMemory((void**)&pvarRetValue->pstrVal, pvarRetValue->strLen);
					TV_VT(pvarRetValue) = VTYPE_BLOB;
					if (pvarRetValue->pstrVal)
					{
						if (SUCCEEDED(pStream->Read(pvarRetValue->pstrVal, pvarRetValue->strLen, 0))) // reading stream to buffer
						{
							Ret = TRUE;
						}
					}
				}
			}
		}
		pStream->Release(); // releasing stream
	}
	return Ret;
}


#endif//__linux__
