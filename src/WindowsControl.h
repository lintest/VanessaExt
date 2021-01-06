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
private:
	HWND hProcessMonitor = NULL;
	void StartProcessMonitoring();
public:
	void OnProcessFinished(DWORD ProcessId, DWORD ExitCode);
#endif//_WINDOWS

private:
	int64_t LaunchProcess(const std::wstring& cmd, bool show);

#ifdef USE_BOOST
	std::unique_ptr<WebSocketBase> webSocket;
	void CloseWebSocket() { webSocket.reset(); }
	static std::wstring WebSocket(const std::string& url, const std::string& msg);
	std::wstring OpenWebSocket(const std::string& url);
	std::wstring SendWebSocket(const std::string& msg);
#endif//USE_BOOST

private:
	static std::vector<std::u16string> names;
	WindowsControl();
public:
	virtual ~WindowsControl() override;
};

#endif //__WINDOWSCONTROL_H__
