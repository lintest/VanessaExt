#include "stdafx.h"
#include "ScreenMngr.h"
#include "json_ext.h"

BOOL ScreenManager::CaptureWindow(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	HWND hWnd = 0;
	if (lSizeArray > 0) hWnd = (HWND)VarToInt(paParams);
	return CaptureWindow(pvarRetValue, hWnd);
}

#ifdef __linux__

#include <X11/extensions/Xrandr.h>
#include "screenshot.h"
#include "XWinBase.h"

class ScreenEnumerator : public WindowHelper
{
public:
	std::wstring Enumerate() {
		int count_screens = ScreenCount(display);
		for (int i = 0; i < count_screens; ++i) {
			Screen* screen = ScreenOfDisplay(display, i);
			JSON j;
			j["Id"] = i;
			j["Width"] = screen->width;
			j["Height"] = screen->height;
			json.push_back(j);
		}
		return *this;
	}
};

class DisplayEnumerator : public WindowHelper
{
public:
	std::wstring Enumerate() {
		int	count;
		Window root = DefaultRootWindow(display);
		XRRMonitorInfo* monitors = XRRGetMonitors(display, root, false, &count);
		if (count == -1) return {};
		for (int i = 0; i < count; ++i) {
			XRRMonitorInfo* info = monitors + i;
			char* name = XGetAtomName(display, info->name);
			JSON j;
			j["Name"] = name;
			j["Left"] = info->x;
			j["Top"] = info->y;
			j["Width"] = info->width;
			j["Height"] = info->height;
			j["mWidth"] = info->mwidth;
			j["mHeight"] = info->mheight;
			j["Right"] = info->x + info->width;
			j["Bottom"] = info->y + info->height;
			j["Automatic"] = (bool)info->automatic;
			j["Primary"] = (bool)info->primary;
			XFree(name);
			json.push_back(j);
		}
		XFree(monitors);
		return *this;
	}
};

std::wstring ScreenManager::GetScreenList()
{
	return ScreenEnumerator().Enumerate();
}

std::wstring ScreenManager::GetDisplayList(tVariant* paParams, const long lSizeArray)
{
	return DisplayEnumerator().Enumerate();
}

BOOL ScreenManager::CaptureScreen(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	return CaptureWindow(pvarRetValue, 0);
}

BOOL ScreenManager::CaptureWindow(tVariant* pvarRetValue, HWND hWnd)
{
	Display* display = XOpenDisplay(NULL);
	if (display == NULL) return false;

	Window window = hWnd;
	if (window == 0) window = DefaultRootWindow(display);

	BOOL success = false;
	XWindowAttributes gwa;
	XGetWindowAttributes(display, window, &gwa);
	XImage* image = XGetImage(display, window, 0, 0, gwa.width, gwa.height, AllPlanes, ZPixmap);
	X11Screenshot screenshot = X11Screenshot(image);
	std::vector<char> buffer;
	if (screenshot.save_to_png(buffer)) {
		pvarRetValue->strLen = buffer.size();
		m_iMemory->AllocMemory((void**)&pvarRetValue->pstrVal, pvarRetValue->strLen);
		TV_VT(pvarRetValue) = VTYPE_BLOB;
		if (pvarRetValue->pstrVal) {
			memcpy((void*)pvarRetValue->pstrVal, &buffer[0], pvarRetValue->strLen);
			success = true;
		}
	}
	XDestroyImage(image);
	XCloseDisplay(display);
	return success;
}

class ScreenHelper : public WindowHelper
{
public:
	ScreenHelper() {
		unsigned long* geometry = NULL;
		unsigned long geometry_size = 0;
		unsigned long* workarea = NULL;
		unsigned long workarea_size = 0;
		Window root = DefaultRootWindow(display);
		if (!GetProperty(root, XA_CARDINAL, "_NET_DESKTOP_GEOMETRY", VXX(&geometry), &geometry_size)) return;
		if (!GetProperty(root, XA_CARDINAL, "_NET_WORKAREA", VXX(&workarea), &workarea_size)) {
			if (!GetProperty(root, XA_CARDINAL, "_WIN_WORKAREA", VXX(&workarea), &workarea_size)) {
				XFree(geometry);
				return;
			}
		};
		if (geometry_size >= 2) {
			json["Left"] = json["Top"] = 0;
			json["Right"] = json["Width"] = geometry[0];
			json["Bottom"] = json["Height"] = geometry[1];
		}
		if (workarea_size >= 4) {
			JSON j;
			j["Left"] = workarea[0];
			j["Top"] = workarea[1];
			j["Width"] = workarea[2];
			j["Height"] = workarea[3];
			json["Work"] = j;
		}
		XFree(geometry);
		XFree(workarea);
	}
};

std::wstring ScreenManager::GetScreenInfo()
{
	ScreenHelper helper;
	if (helper) return helper;

	JSON json;
	Display* display = XOpenDisplay(NULL);
	if (!display) return {};
	int number = DefaultScreen(display);
	json["Left"] = json["Top"] = 0;
	json["Right"] = json["Width"] = DisplayWidth(display, number);
	json["Bottom"] = json["Height"] = DisplayHeight(display, number);
	XCloseDisplay(display);
	return json;
}

std::wstring ScreenManager::GetDisplayInfo(tVariant* paParams, const long lSizeArray)
{
	return {};
}

#else//__linux__

nlohmann::json RectToJson(const RECT& rect)
{
	nlohmann::json json;
	json["Left"] = rect.left;
	json["Top"] = rect.top;
	json["Right"] = rect.right;
	json["Bottom"] = rect.bottom;
	json["Width"] = rect.right - rect.left;
	json["Height"] = rect.bottom - rect.top;
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

	nlohmann::json json;
	BOOL bResult = ::EnumDisplayMonitors(hdc, lpRect, [](HMONITOR hMonitor, HDC, LPRECT rect, LPARAM lParam) -> BOOL
		{
			nlohmann::json j = RectToJson(*rect);
			MONITORINFOEX mi;
			mi.cbSize = sizeof(mi);
			if (::GetMonitorInfo(hMonitor, &mi)) {
				j["Name"] = WC2MB((mi.szDevice));
				j["Work"] = RectToJson(mi.rcWork);
			}
			nlohmann::json* json = (JSON*)lParam;
			json->push_back(j);
			return TRUE;
		}, (LPARAM)&json);

	if (hdc) ReleaseDC(hWnd, hdc);
	return MB2WC(json.dump());
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

	nlohmann::json json = RectToJson(mi.rcMonitor);
	json["Name"] = WC2MB((mi.szDevice));
	json["Work"] = RectToJson(mi.rcWork);
	return MB2WC(json.dump());
}

std::wstring ScreenManager::GetScreenInfo()
{
	RECT rect, work{ 0,0,0,0 };
	rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
	rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
	nlohmann::json json = RectToJson(rect);
	json["Work"] = RectToJson(work);
	return MB2WC(json.dump());
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
