#ifndef __PROCMNG_H__
#define __PROCMNG_H__

#include <types.h>
#include <string>
#include "IMemoryManager.h"

class WebSocketBase;

class ProcessManager {
public:
	static std::wstring FindTestClient(long port);
	static std::wstring GetProcessList(bool only1c);
	static std::wstring GetProcessInfo(long pid);
	static std::wstring FindProcess(const std::wstring name);
	static std::wstring OpenWebSocket(WebSocketBase** ws, const std::string& url);
	static std::wstring SendWebSocket(WebSocketBase** ws, const std::string& msg);
	static std::wstring ProcessManager::WebSocket(const std::string& url, const std::string& msg);
	static void Sleep(long interval);
	static int64_t ProcessId();
};

#endif //__PROCMNG_H__
