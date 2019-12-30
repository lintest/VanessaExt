#include "stdafx.h"
#include "ProcMngr.h"
#include "json_ext.h"

#ifdef __linux__

#include <fstream>
#include <dirent.h>
#include <sys/stat.h>

#define PROC_DIRECTORY "/proc/"

class ProcList
{
private:
    int IsNumeric(const char* ccharptr_CharacterList)
    {
        for ( ; *ccharptr_CharacterList; ccharptr_CharacterList++)
            if (*ccharptr_CharacterList < '0' || *ccharptr_CharacterList > '9')
                return 0; // false
        return 1; // true
    }

    std::string text(const char *dir, const char *name) {
        std::string path = std::string(PROC_DIRECTORY) + dir + name;
        std::ifstream file(path.c_str());
        std::string text;
        if (file) std::getline(file, text);
        return text;
    }

    std::string date(const char *dir) {
        struct stat st;
        std::string path = std::string(PROC_DIRECTORY) + dir;
        stat(path.c_str(), &st);
        time_t t = st.st_mtime;
        struct tm lt;
        localtime_r(&t, &lt);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &lt);
        return buffer;
    }

    JSON json;

public:
    ProcList() {
        struct dirent* dirEntity = NULL ;
        DIR* dir_proc = NULL ;

        dir_proc = opendir(PROC_DIRECTORY) ;
        if (dir_proc == NULL)
        {
            perror("Couldn't open the " PROC_DIRECTORY " directory") ;
            return;
        }

        while ((dirEntity = readdir(dir_proc)) != 0) {
            if (dirEntity->d_type == DT_DIR) {
                if (IsNumeric(dirEntity->d_name)) {
                    JSON j;
                    std::string comm = text(dirEntity->d_name, "/comm");
                    if (comm.substr(0, 4) != "1cv8") continue;
                    std::string line = text(dirEntity->d_name, "/cmdline");
                    for (int i = 0; i < line.size(); i++) {
                        if (line[i] == 0) line[i] = ' ';
                    }
                    j["ProcessId"] = std::stoi(dirEntity->d_name);
                    j["CreationDate"] = date(dirEntity->d_name);
                    j["CommandLine"] = line;
                    json.push_back(j);
                }
            }
        }
        closedir(dir_proc) ;
    }

    std::string dump() {
        return json.dump();
    }
};

std::wstring ProcessManager::GetProcessList(tVariant* paParams, const long lSizeArray)
{
	return MB2WC(ProcList().dump());
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
