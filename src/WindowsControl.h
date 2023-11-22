#pragma once
#include "stdafx.h"
#include "BaseHelper.h"
#include "VideoPainter.h"
#include "WebSocket.h"

#ifdef _WINDOWS
class WinUIAuto;
#endif//_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// class WindowsControl
class WindowsControl : public BaseHelper
{
#ifdef _WINDOWS
private:
	HWND hProcessMonitor = NULL;
	void StartProcessMonitoring();
	std::string GetElements(const VH& id, int64_t level);
	int64_t GetScaleFactor(int64_t window);
	std::unique_ptr<WinUIAuto> m_automation;
public:
	void OnProcessFinished(DWORD ProcessId, DWORD ExitCode);
	WinUIAuto& getUIAuto();
#endif//_WINDOWS

private:
	int64_t LaunchProcess(const std::wstring& cmd, bool show);
	void ExitCurrentProcess(int64_t status);

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
