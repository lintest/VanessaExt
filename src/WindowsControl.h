#pragma once
#include "stdafx.h"
#include "BaseHelper.h"
#include "VideoPainter.h"
#include "WebSocket.h"

#ifdef _WINDOWS

#include <uiautomation.h>

template<typename T>
struct UIAutoDeleter {
	void operator()(T* a) { if (a) a->Release(); }
};

template<typename T>
using UIAutoUniquePtr = std::unique_ptr<T, UIAutoDeleter<T>>;

#endif//_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// class WindowsControl
class WindowsControl : public BaseHelper
{
#ifdef _WINDOWS
private:
	HWND hProcessMonitor = NULL;
	void StartProcessMonitoring();
	std::string GetElements(int64_t pid);
	JSON info(IUIAutomationElement* element);
	UIAutoUniquePtr<IUIAutomation> pAutomation;
public:
	void OnProcessFinished(DWORD ProcessId, DWORD ExitCode);
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
