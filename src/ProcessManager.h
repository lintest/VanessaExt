#pragma once
#include <types.h>
#include <string>
#include "IMemoryManager.h"

class ProcessManager {
#ifdef _WINDOWS
protected:
	static DWORD ParentProcessId(DWORD pid);
#endif //_WINDOWS
public:
	static std::wstring FindTestClient(int64_t port);
	static std::wstring GetProcessList(bool only1c);
	static std::wstring GetProcessInfo(int64_t pid);
	static std::wstring FindProcess(const std::wstring& name);
	static std::string GetFreeDiskSpace(const std::wstring& disk);
	static bool ConsoleOut(const std::wstring& text, int64_t encoding);
	static void Sleep(int64_t interval);
	static int64_t ProcessId();
};
