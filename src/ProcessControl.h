#pragma once
#include "stdafx.h"
#include "BaseHelper.h"

#include <mutex>

#ifdef _WINDOWS
#include <windows.h>
#include <stdio.h>
#else
#include <sys/types.h>
#endif //_WINDOWS

class ProcessControl : public BaseHelper
{
private:
	static std::vector<std::u16string> names;
	ProcessControl();
	virtual ~ProcessControl() override;
private:
#ifdef _WINDOWS
	STARTUPINFO si{ 0 };
	PROCESS_INFORMATION pi{ 0 };
	HANDLE hInPipeR, hInPipeW, hOutPipeR, hOutPipeW;
#else
	int m_pipe[2] = { -1, -1 };
	pid_t m_pid = -1;
	pid_t m_processGroupId = -1;
	bool m_exited = false;
	bool m_hasExitCode = false;
	int m_exitCode = 0;
	std::mutex m_mutex;
	int PollStateLocked();
	int PollState();
	void ClosePipe();
#endif //_WINDOWS
	bool Create(std::wstring command, bool show);
	bool Terminate();
	bool Input(const std::string& text);
	void Sleep(int64_t msec);
	int64_t Wait(int64_t msec);
	int64_t ProcessId();
	int64_t ExitCode();
	bool IsActive();
};
