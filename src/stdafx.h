// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef __STDAFX_H__
#define __STDAFX_H__

#include <locale>
#include <string>
#include <vector>
#include <types.h>

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"

#ifdef _WINDOWS
#include <windows.h>
#else
typedef int64_t HWND;
#endif //_WINDOWS

#if defined(__linux__) || defined(__APPLE__)
#define LINUX_OR_MACOS
#endif

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);

std::wstring MB2WC(const std::string& source);
std::string WC2MB(const std::wstring& source);

std::wstring VarToStr(tVariant* paParams);
int32_t VarToInt(tVariant* paParams);

#ifdef _WINDOWS
HWND VarToHwnd(tVariant* paParams);
#endif //_WINDOWS

class AddInNative : public IComponentBase
{
public:
	bool ADDIN_API AllocMemory(void** pMemory, unsigned long ulCountByte) const;
	// IInitDoneBase
	bool ADDIN_API Init(void*) override;
	bool ADDIN_API setMemManager(void* mem) override;
	long ADDIN_API GetInfo() override;
	void ADDIN_API Done() override;
private:
	IMemoryManager* m_iMemory = nullptr;
protected:
	IAddInDefBase* m_iConnect = nullptr;
};

#endif //__STDAFX_H__
