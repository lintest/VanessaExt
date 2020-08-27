#include "ProcessControl.h"

std::vector<std::u16string> ProcessControl::names = {
	AddComponent(u"ProcessControl", []() { return new ProcessControl; }),
};

ProcessControl::ProcessControl()
{
	AddProperty(u"ProcessId", u"ИдентификаторПроцесса",
		[&](VH var) { var = this->ProcessId(); }
	);
	AddProperty(u"ExitCode", u"КодВозврата",
		[&](VH var) { var = this->ExitCode(); }
	);
	AddProperty(u"IsActive", u"Активный",
		[&](VH var) { var = this->IsActive(); }
	);

	AddFunction(u"Create", u"Создать",
		[&](VH command, VH show) { this->result = this->Create(command, show); },
		{ {1, false} }
	);

	AddFunction(u"Wait", u"Ждать", 
		[&](VH msec) { this->result = this->Wait(msec); }
	);

	AddFunction(u"InputData", u"ВвестиДанные",
		[&](VH text) { this->result = this->Input(text); },
		{ {1, false} }
	);

	AddFunction(u"Terminate", u"Прервать"
		, [&]() { this->result = this->Terminate(); }
	);

	AddProcedure(u"Sleep", u"Пауза", [&](VH msec) { this->Sleep(msec); });

#ifdef _WINDOWS
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	CreatePipe(&hInPipeR, &hInPipeW, &saAttr, 0);
	CreatePipe(&hOutPipeR, &hOutPipeW, &saAttr, 0);
	SetHandleInformation(hInPipeW, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hOutPipeR, HANDLE_FLAG_INHERIT, 0);
#endif //_WINDOWS
}

#ifdef _WINDOWS

ProcessControl::~ProcessControl()
{
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

bool ProcessControl::Create(std::wstring command, bool show)
{
	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdInput = hInPipeR;
	si.dwFlags = STARTF_USESTDHANDLES;
	if (!show) {
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}
	return ::CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
}

bool ProcessControl::Terminate()
{
	return ::TerminateProcess(pi.hProcess, 0);
}

int64_t ProcessControl::Wait(int64_t msec)
{
	if (msec == 0) msec = INFINITE;
	switch (::WaitForSingleObject(pi.hProcess, (DWORD)msec)) {
	case 0: return 0;
	case WAIT_TIMEOUT: return 1;
	default: return -1;
	}
}

bool ProcessControl::Input(const std::string &text)
{
	DWORD dwWritten;
	return ::WriteFile(hInPipeW, text.c_str(), (DWORD)text.size(), &dwWritten, NULL);
}

void ProcessControl::Sleep(int64_t msec)
{
	::Sleep((DWORD)msec);
}

int64_t ProcessControl::ProcessId()
{
	return (int32_t)pi.dwProcessId;
}

int64_t ProcessControl::ExitCode()
{
	DWORD code;
	::GetExitCodeProcess(pi.hProcess, &code);
	return (int32_t)code;
}

bool ProcessControl::IsActive()
{
	DWORD code;
	::GetExitCodeProcess(pi.hProcess, &code);
	return code == STILL_ACTIVE;
}

#else //_WINDOWS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

ProcessControl::ProcessControl()
{
	pipe(m_pipe);
}

ProcessControl::~ProcessControl()
{
	close(m_pipe[PIPE_READ]);
	close(m_pipe[PIPE_WRITE]);
}

bool ProcessControl::Create(std::wstring command, bool show)
{
	std::string cmd = WC2MB(command);
	int child = fork();
	if (0 == child) {
		if (dup2(m_pipe[PIPE_READ], STDIN_FILENO) == -1) exit(errno);
		close(m_pipe[PIPE_READ]);
		close(m_pipe[PIPE_WRITE]);
		int result = execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), NULL);		
		exit(result);
	}
	else if (child > 0) {
		close(m_pipe[PIPE_READ]);
	}
	else {
		return false;
	}
	return true;
}

bool ProcessControl::Terminate()
{
	return false;
}

bool ProcessControl::Input(const std::string& text)
{
	auto res = write(m_pipe[PIPE_WRITE], text.data(), text.size());	
	return res >= 0;
}

bool ProcessControl::Sleep(int64_t msec)
{
	unsigned long ms = (unsigned long)msec;
	::usleep(ms * 1000);
	return true;
}

int64_t ProcessControl::ProcessId()
{
	return 0;
}

int64_t ProcessControl::ExitCode()
{
	return 0;
}

bool ProcessControl::IsActive()
{
	return false;
}

#endif //_WINDOWS
