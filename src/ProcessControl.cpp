#include "ProcessControl.h"

#ifndef _WINDOWS

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#endif //_WINDOWS

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
	AddFunction(u"Connect", u"Подключить",
		[&](VH pid) { this->Connect(pid); },
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
	AddProcedure(u"Sleep", u"Пауза", 
		[&](VH msec) { this->Sleep(msec); }
	);

#ifdef _WINDOWS
	SECURITY_ATTRIBUTES saAttr = { 0 };
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	CreatePipe(&hInPipeR, &hInPipeW, &saAttr, 0);
	CreatePipe(&hOutPipeR, &hOutPipeW, &saAttr, 0);
	SetHandleInformation(hInPipeW, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hOutPipeR, HANDLE_FLAG_INHERIT, 0);
#else
	pipe(m_pipe);
#endif //_WINDOWS
}

#ifdef _WINDOWS

static WCHAR_T* T(const std::u16string& text)
{
	return (WCHAR_T*)text.c_str();
}

static DWORD WINAPI ProcessThreadProc(LPVOID lpParam)
{
	auto component = (ProcessControl*)lpParam;
	std::string pid = std::to_string(component->pi.dwProcessId);
	std::u16string data = component->MB2WCHAR(pid);
	std::u16string name = component->fullname();
	std::u16string msg = PROCESS_FINISHED;
	auto connecttion = component->connecttion();
	WaitForSingleObject(component->pi.hProcess, INFINITE);
	if (connecttion) connecttion->ExternalEvent(T(name), T(msg), T(data));
	return 0;
}

ProcessControl::~ProcessControl()
{
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

int64_t ProcessControl::Create(std::wstring command, bool show)
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
	auto ok = CreateProcess(NULL, (LPWSTR)command.c_str(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
	CreateThread(0, NULL, ProcessThreadProc, (LPVOID)this, NULL, NULL);
	return ok ? (int64_t)pi.dwProcessId : 0;
}

void ProcessControl::Connect(int64_t pid)
{

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

bool ProcessControl::Input(const std::string& text)
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
	return (int64_t)pi.dwProcessId;
}

int64_t ProcessControl::ExitCode()
{
	DWORD code;
	::GetExitCodeProcess(pi.hProcess, &code);
	return (int64_t)code;
}

bool ProcessControl::IsActive()
{
	DWORD code;
	::GetExitCodeProcess(pi.hProcess, &code);
	return code == STILL_ACTIVE;
}

#else //_WINDOWS

#define PIPE_READ 0
#define PIPE_WRITE 1

ProcessControl::~ProcessControl()
{
	close(m_pipe[PIPE_READ]);
	close(m_pipe[PIPE_WRITE]);
}

int64_t ProcessControl::Create(std::wstring command, bool show)
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

void ProcessControl::Connect(int64_t child)
{
	int waiter = fork();
	if (0 == waiter) {
		int status;
		waitpid(child, &status, WUNTRACED);
		std::string pid = std::to_string(child);
		ExternalEvent(PROCESS_FINISHED, MB2WCHAR(pid));
	}
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

void ProcessControl::Sleep(int64_t msec)
{
	unsigned long ms = (unsigned long)msec;
	::usleep(ms * 1000);
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
