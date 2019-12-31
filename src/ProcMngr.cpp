#include "stdafx.h"
#include "ProcMngr.h"
#include "json_ext.h"

#ifdef __linux__

#include "XWinBase.h"

static std::string file2str(const std::string &filepath) {
	std::ifstream file(filepath);
	std::string text;
	if (file) std::getline(file, text);
	return text;
}

class ClientFinder : public WindowEnumerator
{
private:	
	unsigned long m_result = 0;
	int m_port;

protected:
    virtual bool EnumWindow(Window window) {
		std::string str = GetWindowClass(window);
		if (str.substr(0, 4) != "1cv8") return true;
		unsigned long pid =  GetWindowPid(window);
		if (NotFound(pid)) return true;
		json["window"] = m_result = (unsigned long)window;
		json["title"] = GetWindowTitle(window);
		json["pid"] = pid;
		return false;
	}

public:
    ClientFinder(int port): m_port(port) {}
	operator bool() const { return m_result; }
	operator unsigned long() const { return m_result; }

private:
	bool NotFound(unsigned long pid) {
		std::string filepath = "/proc/";
		filepath.append(to_string(pid)).append("/cmdline");
		std::string line = file2str(filepath);
		std::string port = to_string(m_port);
		char* first = NULL;
		char* second = &line[0];
		for (int i = 0; i < line.size() - 1; i++) {
			if (line[i] != 0) continue;
			first = second;
			second = &line[0] + i + 1;
			if (strcmp(first, "-TPort") == 0 
				&& strcmp(port.c_str(), second) == 0) 
					return false;
		}
		return true;
	}
};

unsigned long ProcessManager::FindTestClient(tVariant* paParams, const long lSizeArray, std::wstring& result)
{
	if (lSizeArray < 1) return 0;
	int port = VarToInt(paParams);
	ClientFinder finder(port);
	std::wstring json = finder.Enumerate();
	if (finder) result.assign(json);
	return finder;
}

class ProcessList : public WindowEnumerator
{
protected:
    virtual bool EnumWindow(Window window) {
		unsigned long pid =  GetWindowPid(window);
		std::string filepath = "/proc/";
		filepath.append(to_string(pid)).append("/cmdline");
		std::string line = file2str(filepath);
		for ( std::string::iterator it = line.begin(); it != line.end(); ++it) {
    		if (*it == 0) *it = ' ';
		}
		JSON j;
		j["window"] = (unsigned long)window;
		j["ProcessId"] = pid;
		j["CommandLine"] = line;
		json.push_back(j);
		return true;
	}
};

std::wstring ProcessManager::GetProcessList(tVariant* paParams, const long lSizeArray)
{
	ProcessList list;
	return list.Enumerate();
}

#else//__linux__

#define _WIN32_DCOM
#include <iostream>
#include <sstream>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

class ProcessEnumerator {
private:
	HRESULT hInitialize;
	IWbemLocator* pLoc = NULL;
	IWbemServices* pSvc = NULL;
	IEnumWbemClassObject* pEnumerator = NULL;
	IWbemClassObject* pclsObj = NULL;
	JSON result;
private:
	BOOL CheckError(HRESULT hres)
	{
		if (SUCCEEDED(hres)) return false;
		result.clear();
		result["error"] = WC2MB(_com_error(hres).ErrorMessage());
		return true;
	}
public:
	ProcessEnumerator(const WCHAR* query)
	{
		hInitialize = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
		if (CheckError(hInitialize)) return;

		HRESULT hres;

		hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
		if (CheckError(hres)) return;

		hres = pLoc->ConnectServer(
			_bstr_t(L"\\\\.\\root\\CIMV2"),      // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (e.g. Kerberos)
			0,                       // Context object
			&pSvc                    // pointer to IWbemServices proxy
		);
		if (CheckError(hres)) return;

		// Set security levels on the proxy -------------------------
		hres = CoSetProxyBlanket(
			pSvc,                        // Indicates the proxy to set
			RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
			RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
			NULL,                        // Server principal name
			RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
			RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
			NULL,                        // client identity
			EOAC_NONE                    // proxy capabilities
		);
		if (CheckError(hres)) return;

		hres = pSvc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(query),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
		if (CheckError(hres)) return;

		ULONG uReturn = 0;
		while (pEnumerator)
		{
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
			if (0 == uReturn || CheckError(hr)) break;

			SAFEARRAY* pNames = NULL;
			pclsObj->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &pNames);

			JSON json;
			long lbound = 0, ubound = 0;
			hr = SafeArrayGetLBound(pNames, 1, &lbound);
			hr = SafeArrayGetUBound(pNames, 1, &ubound);
			for (long i = lbound; i <= ubound; ++i)
			{
				wchar_t* name = NULL;
				hr = SafeArrayGetElement(pNames, &i, &name);
				if (FAILED(hr)) continue;

				VARIANT vtProp;
				hr = pclsObj->Get(name, 0, &vtProp, 0, 0);
				if (SUCCEEDED(hr)) {
					switch (vtProp.vt) {
					case VT_BSTR:
						json[WC2MB(name)] = WC2MB(vtProp.bstrVal);
						break;
					case VT_I4:
						json[WC2MB(name)] = vtProp.intVal;
						break;
					default:
						VARIANTARG vtDest;
						hr = VariantChangeType(&vtDest, &vtProp, VARIANT_ALPHABOOL, VT_BSTR);
						if SUCCEEDED(hr) json[WC2MB(name)] = WC2MB(vtDest.bstrVal);
					}
				}
				VariantClear(&vtProp);
			}
			result.push_back(json);
			pclsObj->Release();
			pclsObj = NULL;
		}
	}

	~ProcessEnumerator() {
		if (pEnumerator) pEnumerator->Release();
		if (pSvc) pSvc->Release();
		if (pLoc) pLoc->Release();
		if (SUCCEEDED(hInitialize)) CoUninitialize();
	}

	operator std::wstring() {
		return result;
	}
	JSON json() {
		return result;
	}
};

DWORD ProcessManager::ProcessId()
{
	return(GetCurrentProcessId());
}

std::wstring ProcessManager::GetProcessList(tVariant* paParams, const long lSizeArray)
{
	std::wstring query;
	query.append(L"SELECT ProcessId,CreationDate,CommandLine");
	query.append(L" FROM Win32_Process ");
	if (lSizeArray > 0 && paParams->vt == VTYPE_PWSTR && paParams->pwstrVal) {
		query.append(L" WHERE Name LIKE '%");
		query.append(paParams->pwstrVal);
		query.append(L"%'");
	}
	return ProcessEnumerator(query.c_str());
}

std::wstring ProcessManager::GetProcessInfo(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return {};
	std::wstring query;
	query.append(L"SELECT * FROM Win32_Process WHERE ProcessId=");
	query.append(std::to_wstring(VarToInt(paParams)));
	JSON json = ProcessEnumerator(query.c_str()).json();
	if (json.is_array() && json.size() == 1) {
		return MB2WC(json[0].dump());
	}
	return {};
}

std::wstring ProcessManager::FindProcess(tVariant* paParams, const long lSizeArray)
{
	if (lSizeArray < 1) return {};
	if (paParams->vt != VTYPE_PWSTR) return {};
	if (paParams->pwstrVal == NULL) return {};
	return ProcessEnumerator(paParams->pwstrVal);
}

unsigned long ProcessManager::FindTestClient(tVariant* paParams, const long lSizeArray, std::wstring& result)
{
	if (lSizeArray < 1) return NULL;
	std::wstring query;
	query.append(L"SELECT * FROM Win32_Process");
	query.append(L" WHERE Name LIKE '1cv8%' ");
	query.append(L" AND CommandLine LIKE '% /TESTCLIENT %'");
	query.append(L" AND CommandLine LIKE '% -TPort ");
	query.append(std::to_wstring(VarToInt(paParams))).append(L" %'");
	JSON json = ProcessEnumerator(query.c_str()).json();
	if (!json.is_array() || json.empty()) return NULL;

	std::pair<HWND, DWORD> params = { 0, (DWORD)json[0]["ProcessId"] };
	// Enumerate the windows using a lambda to process each window
	BOOL bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			auto pParams = (std::pair<HWND, DWORD>*)(lParam);
			wchar_t buffer[256];
			DWORD pid;
			if (IsWindowVisible(hWnd)
				&& ::GetWindowThreadProcessId(hWnd, &pid)
				&& pid == pParams->second
				&& ::GetClassName(hWnd, buffer, 256)
				&& wcscmp(L"V8TopLevelFrameSDI", buffer) == 0
				) {
				// Stop enumerating
				SetLastError(-1);
				pParams->first = hWnd;
				return FALSE;
			}
			// Continue enumerating
			return TRUE;
		}, (LPARAM)&params);

	if (!bResult && GetLastError() == -1 && params.first)
	{
		result = MB2WC(json[0].dump());
		return (unsigned long)params.first;
	}
	return 0;
}

#endif//__linux__
