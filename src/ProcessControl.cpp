#include "ProcessControl.h"

const wchar_t* ProcessControl::m_ExtensionName = L"ProcessControl";

const std::vector<AddInBase::Alias> ProcessControl::m_PropList{
	Alias(eProcessId , false , L"ProcessId" , L"ИдентификаторПроцесса"),
	Alias(eExitCode  , false , L"ExitCode"  , L"КодВозврата"),
	Alias(eIsActive  , false , L"IsActive"  , L"Активный"),
};

const std::vector<AddInBase::Alias> ProcessControl::m_MethList{
	Alias(eCreate    , 3, false, L"Create"     , L"Создать"),
	Alias(eWait      , 1, false, L"Wait"       , L"Ждать"),
	Alias(eTerminare , 0, false, L"Terminate"  , L"Прервать"),
	Alias(eInputData , 1, false, L"InputData"  , L"ВвестиДанные"),
};

bool ProcessControl::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum) {
	case eProcessId:
		return VA(pvarPropVal) << ProcessId();
	case eExitCode:
		return VA(pvarPropVal) << ExitCode();
	case eIsActive:
		return VA(pvarPropVal) << IsActive();
	default:
		return false;
	}
}

bool ProcessControl::SetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum) {
	default:
		return false;
	}
}

//---------------------------------------------------------------------------//
bool ProcessControl::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case eCreate:
		return Create(paParams, lSizeArray);
	case eTerminare:
		return Terminate(paParams, lSizeArray);
	case eInputData:
		return Input(paParams, lSizeArray);
	case eWait:
		return Wait(paParams, lSizeArray);
	case eSleep:
		return Sleep(paParams, lSizeArray);
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool ProcessControl::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum) {
	case eWait:
		return VA(pvarRetValue) << Wait(paParams, lSizeArray);
	default:
		return false;
	}
}

static bool DefInt(tVariant* pvar, int value = 0)
{
	TV_VT(pvar) = VTYPE_I4;
	TV_I4(pvar) = value;
	return true;
}

bool ProcessControl::GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue)
{
	switch (lMethodNum) {
	case eCreate: if (lParamNum > 0) return DefInt(pvarParamDefValue);
	}
	return false;
}

#ifdef _WINDOWS

ProcessControl::ProcessControl()
{
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	CreatePipe(&hInPipeR, &hInPipeW, &saAttr, 0);
	CreatePipe(&hOutPipeR, &hOutPipeW, &saAttr, 0);

	SetHandleInformation(hInPipeW, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hOutPipeR, HANDLE_FLAG_INHERIT, 0);

}

ProcessControl::~ProcessControl()
{
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

bool ProcessControl::Create(tVariant* paParams, const long lSizeArray)
{
	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdInput = hInPipeR;
	si.dwFlags = STARTF_USESTDHANDLES;
	if (VarToInt(paParams + 1) == 0) {
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}
	WcharWrapper wsCommandLine = VarToStr(paParams).c_str();
	return ::CreateProcessW(NULL, wsCommandLine, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
}

bool ProcessControl::Terminate(tVariant* paParams, const long lSizeArray)
{
	return ::TerminateProcess(pi.hProcess, 0);
}

int32_t ProcessControl::Wait(tVariant* paParams, const long lSizeArray)
{
	DWORD msec = VarToInt(paParams);
	if (msec == 0) msec = INFINITE;
	switch (::WaitForSingleObject(pi.hProcess, msec)) {
	case 0: return 0;
	case WAIT_TIMEOUT: return 1;
	default: return -1;
	}
}

bool ProcessControl::Input(tVariant* paParams, const long lSizeArray)
{
	DWORD dwWritten;
	std::wstring text = VarToStr(paParams);
	return ::WriteFile(hInPipeW, text.c_str(), (DWORD)text.size(), &dwWritten, NULL);
}

bool ProcessControl::Sleep(tVariant* paParams, const long lSizeArray)
{
	::Sleep(VarToInt(paParams));
	return true;
}

int32_t ProcessControl::ProcessId()
{
	return (int32_t)pi.dwProcessId;
}

int32_t ProcessControl::ExitCode()
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

bool ProcessControl::Create(tVariant *paParams, const long lSizeArray)
{
	std::string cmd = WC2MB(VarToStr(paParams));
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

bool ProcessControl::Terminate(tVariant* paParams, const long lSizeArray)
{
	return false;
}

bool ProcessControl::Input(tVariant* paParams, const long lSizeArray)
{
	std::string text = WC2MB(VarToStr(paParams));
	auto res = write(m_pipe[PIPE_WRITE], text.data(), text.size());	
	return res >= 0;
}

bool ProcessControl::Sleep(tVariant* paParams, const long lSizeArray)
{
	unsigned long ms = VarToInt(paParams);
	::usleep(ms * 1000);
	return true;
}

int32_t ProcessControl::ProcessId()
{
	return 0;
}

int32_t ProcessControl::ExitCode()
{
	return 0;
}

bool ProcessControl::IsActive()
{
	return false;
}

#endif //_WINDOWS
