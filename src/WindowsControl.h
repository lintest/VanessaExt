#ifndef __WINDOWSCONTROL_H__
#define __WINDOWSCONTROL_H__

#include "stdafx.h"
#include "AddInBase.h"
#include "WebSocket.h"

///////////////////////////////////////////////////////////////////////////////
// class WindowsControl
class WindowsControl : public AddInBase
{
private:
	enum Props
	{
		eClipboardImage = 0,
		eClipboardText,
		eClipboardFormat,
		eActiveWindow,
		eProcessId,
		eProcessList,
		eWindowList,
		eDisplayList,
		eScreenInfo,
		eCursorPos,
		eVersion,
	};

	enum Methods
	{
		eFindTestClient = 0,
		eGetProcessList,
		eGetProcessInfo,
		eGetProcessWindow,
		eActivateProcess,
		eGetDisplayList,
		eGetDisplayInfo,
		eGetScreenInfo,
		eGetWindowList,
		eGetWindowInfo,
		eGetWindowSize,
		eTakeScreenshot,
		eCaptureWindow,
		eCaptureProcess,
		eEnableResizing,
		eSetWindowPos,
		eSetWindowSize,
		eSetWindowState,
		eActivateWindow,
		eMaximizeWindow,
		eMinimizeWindow,
		eRestoreWindow,
		eEmptyClipboard,
		eGetCursorPos,
		eSetCursorPos,
		eEmulateClick,
		eEmulateDblClick,
		eEmulateHotkey,
		eEmulateMouse,
		eEmulateWheel,
		eEmulateText,
		eOpenWebSocket,
		eSendWebSocket,
		eCloseWebSocket,
		eWebSocket,
		eFindFiles,
		eSleep,
	};

	WebSocketBase* webSocket = nullptr;
	static const wchar_t* m_ExtensionName;
	static const std::vector<Alias> m_PropList;
	static const std::vector<Alias> m_MethList;
	const wchar_t* ExtensionName() const override { return m_ExtensionName; };
	const std::vector<Alias>& PropList() const override { return m_PropList; };
	const std::vector<Alias>& MethList() const override { return m_MethList; };
	bool CloseWebSocket() { if (webSocket) delete webSocket; webSocket = nullptr; return true; }
public:
	virtual ~WindowsControl() { CloseWebSocket(); }
	bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal) override;
	bool ADDIN_API SetPropVal(const long lPropNum, tVariant* pvarPropVal) override;
	bool ADDIN_API CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray) override;
	bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) override;
	bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue) override;
};

#endif //__WINDOWSCONTROL_H__
