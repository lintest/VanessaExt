#include "ProcessControl.h"

const wchar_t* ProcessControl::m_ExtensionName = L"ProcessControl";

const std::vector<AddInBase::Alias> ProcessControl::m_PropList{
	Alias(eVersion  , false , L"Version" , L"Версия"),
};

const std::vector<AddInBase::Alias> ProcessControl::m_MethList{
	Alias(eCreate , 2, false, L"Create" , L"Создать"),
	Alias(eWrite  , 1, false, L"Write"  , L"Записать"),
	Alias(eRead   , 0, false, L"Read"   , L"Прочитать"),
	Alias(eBreak  , 0, false, L"Break"  , L"Прервать"),
};

bool ProcessControl::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum) {
	case eProcessId:
		return VA(pvarPropVal) << ProcessId();
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
	case eBreak:
		return Break(paParams, lSizeArray);
	case eWrite:
		return Write(paParams, lSizeArray);
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool ProcessControl::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum) {
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
	case eCreate: if (lParamNum == 1) return DefInt(pvarParamDefValue);
	}
	return false;
}

#ifdef _WINDOWS

ProcessControl::ProcessControl() 
{
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	CreatePipe(&hInPipeR, &hInPipeW, &saAttr, 0);
	CreatePipe(&hOutPipeR, &hOutPipeW, &saAttr, 0);

	SetHandleInformation(hInPipeW, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hOutPipeR, HANDLE_FLAG_INHERIT, 0);

//	si.hStdError = hOutPipeW;
//	si.hStdOutput = hOutPipeW;
	si.hStdInput = hInPipeR;
	si.dwFlags |= STARTF_USESTDHANDLES;
}

ProcessControl::~ProcessControl() 
{
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

bool ProcessControl::Create(tVariant* paParams, const long lSizeArray)
{
	if (VarToInt(paParams + 1) == 0) {
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}
	WcharWrapper wsCommandLine = VarToStr(paParams).c_str();
	return ::CreateProcessW(NULL, wsCommandLine, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
}

bool ProcessControl::Break(tVariant* paParams, const long lSizeArray)
{
	return ::TerminateProcess(pi.hProcess, 0);
}

bool ProcessControl::Write(tVariant* paParams, const long lSizeArray)
{
	DWORD dwWritten;
	std::wstring text = VarToStr(paParams);
	return WriteFile(hInPipeW, text.c_str(), text.size(), &dwWritten, NULL);
}

int32_t ProcessControl::ProcessId()
{
	return (int32_t)pi.dwProcessId;
}

#else //_WINDOWS

ProcessControl::ProcessControl()
{
}

ProcessControl::~ProcessControl()
{
}

bool ProcessControl::Create(tVariant* paParams, const long lSizeArray)
{
	return false;
}

bool ProcessControl::Break(tVariant* paParams, const long lSizeArray)
{
	return false;
}

bool ProcessControl::Write(tVariant* paParams, const long lSizeArray)
{
	return false;
}

int32_t ProcessControl::ProcessId()
{
	return 0;
}

#endif //_WINDOWS
