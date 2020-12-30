#ifndef __WINDOWSCONTROL_H__
#define __WINDOWSCONTROL_H__

#include "stdafx.h"
#include "BaseHelper.h"
#include "VideoPainter.h"
#include "WebSocket.h"

///////////////////////////////////////////////////////////////////////////////
// class WindowsControl
class WindowsControl : public BaseHelper
{
#ifdef _WINDOWS
	friend DWORD WINAPI ProcessThreadProc(LPVOID lpParam);
#endif//_WINDOWS

private:
	int64_t LaunchProcess(const std::wstring& cmd, bool show);
#ifdef USE_BOOST
	WebSocketBase* webSocket = nullptr;
	bool CloseWebSocket() { if (webSocket) delete webSocket; webSocket = nullptr; return true; }
#else//USE_BOOST
	bool CloseWebSocket() { return true; }
#endif//USE_BOOST

private:
	static std::vector<std::u16string> names;
	WindowsControl();
public:
	virtual ~WindowsControl() override;
};

#endif //__WINDOWSCONTROL_H__
