#ifndef __WINDOWSCONTROL_H__
#define __WINDOWSCONTROL_H__

#include "stdafx.h"
#include "AddInBase.h"

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
		eWebSocket,
		eSleep,
	};

	static const wchar_t* m_ExtensionName;
	static const std::vector<Alias> m_PropList;
	static const std::vector<Alias> m_MethList;
	const wchar_t* ExtensionName() const override { return m_ExtensionName; };
	const std::vector<Alias>& PropList() const override { return m_PropList; };
	const std::vector<Alias>& MethList() const override { return m_MethList; };

public:
	bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal) override;
	bool ADDIN_API SetPropVal(const long lPropNum, tVariant* pvarPropVal) override;
	bool ADDIN_API CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray) override;
	bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) override;
};

#endif //__WINDOWSCONTROL_H__
