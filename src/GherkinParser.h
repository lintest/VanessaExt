#ifndef __GHERKINPARSER_H__
#define __GHERKINPARSER_H__

#include "stdafx.h"
#include "AddInNative.h"

namespace Gherkin {
	class GherkinProvider;
}

class GherkinParser :
	public AddInNative
{
#ifdef _WINDOWS
private:
	HWND hWndMonitor = NULL;
public:
	virtual ~GherkinParser();
	void CreateProgressMonitor();
	void OnProgress(UINT id, const std::string& data);
	void ScanFolder(const std::wstring& path, const std::string& filter);
	void AbortScan();
#endif//_WINDOWS
private:
	static std::vector<std::u16string> names;
	GherkinParser();
private:
	std::unique_ptr<Gherkin::GherkinProvider> provider;
	void ExitCurrentProcess(int64_t status);
};

#endif //__GHERKINPARSER_H__