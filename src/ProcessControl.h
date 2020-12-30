#ifndef __PROCESSCONTROL_H__
#define __PROCESSCONTROL_H__

#include "stdafx.h"
#include "BaseHelper.h"

class ProcessControl : public BaseHelper
{
private:
	static std::vector<std::u16string> names;
	ProcessControl();
	virtual ~ProcessControl() override;
#ifdef _WINDOWS
private:
	STARTUPINFO si{ 0 };
	PROCESS_INFORMATION pi{ 0 };
	HANDLE hInPipeR, hInPipeW, hOutPipeR, hOutPipeW;
	friend DWORD WINAPI ProcessThreadProc(LPVOID lpParam);
#else
private:
	int m_pipe[2] = { 0 , 0 };
#endif //_WINDOWS
	int64_t Create(std::wstring command, bool show);
	bool Terminate();
	bool Input(const std::string& text);
	void Sleep(int64_t msec);
	int64_t Wait(int64_t msec);
	int64_t ProcessId();
	int64_t ExitCode();
	bool IsActive();
};

#endif //__PROCESSCONTROL_H__