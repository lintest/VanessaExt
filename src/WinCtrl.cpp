#include "stdafx.h"

#include "WinCtrl.h"
#include "convertor.h"

#include "json.hpp"
using JSON = nlohmann::json;

std::string WC2MB(const std::wstring &wstr, DWORD locale)
{
	if (wstr.empty()) return {};
	int sz = WideCharToMultiByte(locale, 0, &wstr[0], (int)wstr.size(), 0, 0, 0, 0);
	std::string res(sz, 0);
	WideCharToMultiByte(locale, 0, &wstr[0], (int)wstr.size(), &res[0], sz, 0, 0);
	return res;
}

std::wstring MB2WC(const std::string &str, DWORD locale)
{
	if (str.empty()) return {};
	int sz = MultiByteToWideChar(locale, 0, &str[0], (int)str.size(), 0, 0);
	std::wstring res(sz, 0);
	MultiByteToWideChar(locale, 0, &str[0], (int)str.size(), &res[0], sz);
	return res;
}

std::wstring W(const JSON &json) {
	return MB2WC(json.dump());
}

std::wstring SetWindow::GetWindowList()
{
	JSON json;
	BOOL bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
	{
		if (IsWindowVisible(hWnd)) {
			JSON j;
			j["hWnd"] = (INT64)hWnd;

			WCHAR buffer[256];
			::GetClassName(hWnd, buffer, 256);
			j["class"] = WC2MB(buffer, CP_UTF8);

			int length = GetWindowTextLength(hWnd);
			if (length != 0) {
				std::wstring text;
				text.resize(length);
				::GetWindowText(hWnd, &text[0], length + 1);
				j["text"] = WC2MB(text, CP_UTF8);
			}

			DWORD dwProcessId;
			::GetWindowThreadProcessId(hWnd, &dwProcessId);
			j["pid"] = dwProcessId;

			JSON *json = (JSON*)lParam;
			json->push_back(j);
		}
		return TRUE;
	}, (LPARAM)&json);

	return MB2WC(json.dump(), CP_UTF8);
}

HWND SetWindow::ActiveWindow()
{
	return GetActiveWindow();
}

HWND SetWindow::CurrentWindow()
{
	DWORD pid = GetCurrentProcessId();
	std::pair<HWND, DWORD> params = { 0, pid };

	// Enumerate the windows using a lambda to process each window
	BOOL bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
	{
		auto pParams = (std::pair<HWND, DWORD>*)(lParam);
		WCHAR buffer[256];
		DWORD processId;
		if (IsWindowVisible(hWnd)
			&& ::GetWindowThreadProcessId(hWnd, &processId)
			&& processId == pParams->second
			&& ::GetClassName(hWnd, buffer, 256)
			&& wcscmp(L"V8TopLevelFrameSDI", buffer) == 0
			) {
			// Stop enumerating
			SetLastError(-1);
			pParams->first = hWnd;
			return FALSE;
		}

		// Continue enumerating
		return TRUE;
	}, (LPARAM)&params);

	if (!bResult && GetLastError() == -1 && params.first)
	{
		return params.first;
	}

	return 0;
}

DWORD SetWindow::ProcessId()
{
	return(GetCurrentProcessId());
}

int VarToInt(tVariant* paParams)
{
	return paParams->intVal;
}

HWND VarToHwnd(tVariant* paParams)
{
	return (HWND)IntToPtr(paParams->intVal);
}

BOOL SetWindow::SetWindowSize(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 3) return false;
	HWND hWnd = VarToHwnd(paParams);
	int w = VarToInt(paParams + 1);
	int h = VarToInt(paParams + 2);
	return ::SetWindowPos(hWnd, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

BOOL SetWindow::SetWindowPos(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 3) return false;
	HWND hWnd = VarToHwnd(paParams);
	int x = VarToInt(paParams + 1);
	int y = VarToInt(paParams + 1);
	return ::SetWindowPos(hWnd, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

BOOL SetWindow::ActivateWindow(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return false;
	HWND hWnd = VarToHwnd(paParams);
	return ::SetForegroundWindow(hWnd);
}

BOOL SetWindow::EnableResizing(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 2) return false;
	HWND hWnd = VarToHwnd(paParams);
	BOOL enable = VarToInt(paParams + 1);
	LONG style = ::GetWindowLong(hWnd, GWL_STYLE);
	style = enable ? (style | WS_SIZEBOX) : (style&~WS_SIZEBOX);
	return ::SetWindowLong(hWnd, GWL_STYLE, style);
}

std::wstring SetWindow::GetWindowText(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return false;
	HWND hWnd = VarToHwnd(paParams);
	int length = ::GetWindowTextLength(hWnd);
	std::wstring text;
	if (length != 0) {
		text.resize(length);
		::GetWindowText(hWnd, &text[0], length + 1);
	}
	return text;
}

#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib, "Gdiplus.lib")

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

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

#define AUTO_H(N, T, F)                \
class N {                              \
private:                               \
	T h = NULL;                        \
public:                                \
	N() {}                             \
	N(T h) { this->h = h; }            \
	~N() { if (h) (F); }               \
	operator T() const { return h; }   \
	T* operator &() { return &h; }     \
	T operator->() { return h; }       \
	T operator!() { return !h; }       \
}                                      \
;

AUTO_H(AutoULONG_PTR, ULONG_PTR, GdiplusShutdown(h))
static AutoULONG_PTR gdiplusToken;

#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

BOOL SetWindow::CaptureWindow(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return false;
	HWND hWnd = VarToHwnd(paParams);
	if (hWnd == 0) hWnd = ::GetForegroundWindow();

	RECT rc;
	GetClientRect(hWnd, &rc);
	DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rc, sizeof(rc));

	HDC hdcScreen = GetDC(NULL);
	HDC hDC = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, rc.right - rc.left, rc.bottom - rc.top);
	SelectObject(hDC, hBitmap);

	//Print to memory hdc
	::PrintWindow(hWnd, hDC, 0);

	BOOL Ret = FALSE;
	Gdiplus::GdiplusStartupInput input;
	Gdiplus::GdiplusStartupOutput output;
	Gdiplus::Status status = Gdiplus::Ok;

	if (!gdiplusToken) // initialization of gdi+
	{
		status = Gdiplus::GdiplusStartup(&gdiplusToken, &input, &output);
	}

	if (status == Gdiplus::Ok)
	{
		CLSID clsid;
		GetEncoderClsid(L"image/png", &clsid); // retrieving JPEG encoder CLSID

		Gdiplus::Bitmap SrcBitmap(hBitmap, 0); // creating bitmap

		IStream * pStream;
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
	}
	return Ret;
}
