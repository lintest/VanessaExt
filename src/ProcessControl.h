#ifndef __PROCESSCONTROL_H__
#define __PROCESSCONTROL_H__

#include "stdafx.h"
#include "AddInBase.h"

#ifdef _WINDOWS
#include <windows.h>
#include <stdio.h>
#endif //_WINDOWS

class ProcessControl : public AddInBase
{
private:
	enum Props
	{
		eProcessId,
		eVersion,
	};

	enum Methods
	{
		eCreate = 0,
		eWrite,
		eRead,
		eBreak,
		eSleep,
	};

	static const wchar_t* m_ExtensionName;
	static const std::vector<Alias> m_PropList;
	static const std::vector<Alias> m_MethList;
	const wchar_t* ExtensionName() const override { return m_ExtensionName; };
	const std::vector<Alias>& PropList() const override { return m_PropList; };
	const std::vector<Alias>& MethList() const override { return m_MethList; };
public:
	bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal) override;
	bool ADDIN_API SetPropVal(const long lPropNum, tVariant* pvarPropVal) override;
	bool ADDIN_API CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray) override;
	bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) override;
	bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue);
private:
#ifdef _WINDOWS
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	HANDLE hInPipeR, hInPipeW, hOutPipeR, hOutPipeW;
#endif //_WINDOWS
	bool Create(tVariant* paParams, const long lSizeArray);
	bool Break(tVariant* paParams, const long lSizeArray);
	bool Write(tVariant* paParams, const long lSizeArray);
	int32_t ProcessId();
public:
	ProcessControl();
	~ProcessControl();
};

#endif //__PROCESSCONTROL_H__