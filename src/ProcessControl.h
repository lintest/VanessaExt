#pragma once
#include "stdafx.h"
#include "BaseHelper.h"

#ifdef _WINDOWS
#include <windows.h>
#include <stdio.h>
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
	int m_pipe[2] = { 0 , 0 };
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
