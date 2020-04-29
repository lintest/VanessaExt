#ifndef __PROCMNG_H__
#define __PROCMNG_H__

#include <types.h>
#include <string>
#include "IMemoryManager.h"

class WebSocketBase;

class ProcessManager {
public:
	static std::wstring FindTestClient(tVariant* paParams, const long lSizeArray);
	static std::wstring GetProcessList(tVariant* paParams, const long lSizeArray);
	static std::wstring GetProcessInfo(tVariant* paParams, const long lSizeArray);
	static std::wstring FindProcess(tVariant* paParams, const long lSizeArray);
	static std::wstring OpenWebSocket(WebSocketBase** ws, tVariant* paParams, const long lSizeArray);
	static std::wstring SendWebSocket(WebSocketBase* ws, tVariant* paParams, const long lSizeArray);
	static std::wstring WebSocket(tVariant* paParams, const long lSizeArray);
	static bool Sleep(tVariant* paParams, const long lSizeArray);
	static int64_t ProcessId();
};

#endif //__PROCMNG_H__
