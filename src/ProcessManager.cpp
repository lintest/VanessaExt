#include "stdafx.h"
#include "ProcessManager.h"

#ifdef _WINDOWS

#define _WIN32_DCOM
#include <iostream>
#include <sstream>
#include <comdef.h>
#include <wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "winmm.lib")

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
					if (wcscmp(name, L"CreationDate") == 0) {
						std::wstring str = vtProp.bstrVal;
						str = str.substr(0, 8) + L"T" + str.substr(8, 6);
						json["CreationDate"] = WC2MB(str);
					}
					else {
						switch (vtProp.vt) {
						case VT_NULL:
							break;
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
		return MB2WC(result.dump());
	}
	JSON json() {
		return result;
	}
};

int64_t ProcessManager::ProcessId()
{
	return(GetCurrentProcessId());
}

std::wstring ProcessManager::GetProcessList(bool only1c)
{
	std::wstring query;
	query.append(L"SELECT ProcessId,CreationDate,CommandLine");
	query.append(L" FROM Win32_Process ");
	if (only1c) query.append(L" WHERE Name LIKE '1cv8%'");
	return ProcessEnumerator(query.c_str());
}

std::wstring ProcessManager::GetProcessInfo(int64_t pid)
{
	std::wstring query;
	query.append(L"SELECT * FROM Win32_Process WHERE ProcessId=");
	query.append(std::to_wstring(pid));
	JSON json = ProcessEnumerator(query.c_str()).json();
	if (json.is_array() && json.size() == 1) {
		return MB2WC(json[0].dump());
	}
	return {};
}

std::wstring ProcessManager::FindProcess(const std::wstring& name)
{
	if (name.empty()) return {};
	return ProcessEnumerator(name.c_str());
}

std::wstring ProcessManager::FindTestClient(int64_t port)
{
	std::wstring query;
	query.append(L"SELECT ProcessId,CreationDate,CommandLine");
	query.append(L" FROM Win32_Process ");
	query.append(L" WHERE Name LIKE '1cv8%' ");
	query.append(L" AND CommandLine LIKE '% /TESTCLIENT %'");
	query.append(L" AND CommandLine LIKE '% -TPort ");
	query.append(std::to_wstring(port)).append(L" %'");
	JSON json = ProcessEnumerator(query.c_str()).json();
	if (!json.is_array() || json.empty()) return {};

	using EnumParam = std::pair<DWORD, HWND>;
	EnumParam params{ (DWORD)json[0]["ProcessId"], 0 };
	BOOL bResult = ::EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			auto pParams = (EnumParam*)lParam;
			wchar_t buffer[256];
			DWORD pid;
			if (IsWindowVisible(hWnd)
				&& ::GetWindowThreadProcessId(hWnd, &pid)
				&& pid == pParams->first
				&& ::GetClassName(hWnd, buffer, 256)
				&& wcscmp(L"V8TopLevelFrameSDI", buffer) == 0
				) {
				// Stop enumerating
				SetLastError(-1);
				pParams->second = hWnd;
				return FALSE;
			}
			// Continue enumerating
			return TRUE;
		}, (LPARAM)&params);

	HWND hWnd = params.second;
	if (!bResult && GetLastError() == -1 && hWnd) {
		nlohmann::json j = json[0];
		j["Window"] = (uint64_t)hWnd;
		const int length = GetWindowTextLength(hWnd);
		if (length != 0) {
			std::wstring text;
			text.resize(length);
			::GetWindowText(hWnd, &text[0], length + 1);
			j["Title"] = WC2MB(text);
		}
		return MB2WC(j.dump());
	}
	return {};
}

void ProcessManager::Sleep(int64_t interval)
{
	::Sleep((DWORD)interval);
}

DWORD ProcessManager::ParentProcessId(DWORD pid)
{
	std::wstring query;
	query.append(L"SELECT ParentProcessId");
	query.append(L" FROM Win32_Process ");
	query.append(L" WHERE ProcessId=");
	query.append(std::to_wstring(pid));
	JSON json = ProcessEnumerator(query.c_str()).json();
	if (json.is_array() && json.size() == 1) {
		return json[0]["ParentProcessId"];
	}
	return 0;
}

bool ProcessManager::ConsoleOut(const std::wstring& text, int64_t encoding)
{
	if (text.empty()) return true;
	auto pid = ::GetCurrentProcessId();
	bool attached = false;
	while (!attached) {
		pid = ParentProcessId(pid);
		if (pid == 0) return false;
		attached = AttachConsole(pid);
	}
	bool ok = false;
	auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (encoding == 0 || GetFileType(hConsole) == FILE_TYPE_CHAR) {
		ok = WriteConsole(hConsole, text.c_str(), (DWORD)text.size(), NULL, NULL);
	}
	else {
		auto size = WideCharToMultiByte((UINT)encoding, 0, text.data(), (int)text.size(), NULL, 0, NULL, NULL);
		std::string buffer(size, 0);
		WideCharToMultiByte((UINT)encoding, 0, text.data(), (int)text.size(), &buffer[0], size, NULL, NULL);
		DWORD written;
		ok = WriteFile(hConsole, buffer.data(), (DWORD)size, &written, NULL);
	}
	FreeConsole();
	return ok;
}

#else //_WINDOWS

#include <unistd.h>
#include <sys/stat.h>
#include "XWinBase.h"

static std::string file2str(const std::string& filepath) {
	std::ifstream file(filepath);
	std::string text;
	if (file) std::getline(file, text);
	return text;
}

class ProcessEnumerator : public WindowEnumerator
{
private:
	const std::string proc = "/proc/";

protected:
	std::string GetProcessName(unsigned long pid, bool original = false) {
		std::string filepath = proc + to_string(pid) + "/comm";
		return file2str(filepath);
	}

	std::string GetCommandLine(unsigned long pid, bool original = false) {
		std::string filepath = proc + to_string(pid) + "/cmdline";
		std::string line = file2str(filepath);
		if (original) return line;
		for (std::string::iterator it = line.begin(); it != line.end(); ++it) {
			if (*it == 0) *it = ' ';
		}
		return line;
	}

	std::string GetCreationDate(unsigned long pid) {
		struct stat st;
		std::string filepath = proc + to_string(pid);
		stat(filepath.c_str(), &st);
		time_t t = st.st_mtime;
		struct tm lt;
		localtime_r(&t, &lt);
		char buffer[80];
		strftime(buffer, sizeof(buffer), "%FT%T", &lt);
		return buffer;
	}

};

class ClientFinder : public ProcessEnumerator
{
private:
	int m_port;

protected:
	virtual bool EnumWindow(Window window) {
		std::string str = GetWindowClass(window);
		if (str.substr(0, 4) != "1cv8") return true;
		unsigned long pid = GetWindowPid(window);
		if (NotFound(pid)) return true;
		json["Name"] = GetProcessName(pid);
		json["Window"] = (uint64_t)window;
		json["Title"] = GetWindowTitle(window);
		json["CommandLine"] = GetCommandLine(pid);
		json["CreationDate"] = GetCreationDate(pid);
		json["ProcessId"] = pid;
		return false;
	}

public:
	ClientFinder(int port) : m_port(port) {}

private:
	bool NotFound(unsigned long pid) {
		std::string line = GetCommandLine(pid, true);
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

class ProcessList : public ProcessEnumerator
{
private:
	bool m_filter = false;
	vector<unsigned long> v;
protected:
	virtual bool EnumWindow(Window window) {
		std::string str = GetWindowClass(window);
		if (str.substr(0, 4) != "1cv8") return true;
		unsigned long pid = GetWindowPid(window);
		if (std::find(v.begin(), v.end(), pid) != v.end()) return true;
		v.push_back(pid);
		JSON j;
		j["Name"] = GetProcessName(pid);
		j["Window"] = (uint64_t)window;
		j["Title"] = GetWindowTitle(window);
		j["CommandLine"] = GetCommandLine(pid);
		j["CreationDate"] = GetCreationDate(pid);
		j["ProcessId"] = pid;
		json.push_back(j);
		return true;
	}
public:
	ProcessList(bool filter) : m_filter(filter) {}
};

class ProcessInfo : public ProcessEnumerator
{
protected:
	virtual bool EnumWindow(Window window) { return false; }
public:
	ProcessInfo(unsigned long pid) {
		json["Name"] = GetProcessName(pid);
		json["CommandLine"] = GetCommandLine(pid);
		json["CreationDate"] = GetCreationDate(pid);
		json["ProcessId"] = pid;
	}
};

int64_t ProcessManager::ProcessId()
{
	return getpid();
}

std::wstring ProcessManager::FindTestClient(int64_t port)
{
	return MB2WC(ClientFinder(port).Enumerate());
}

std::wstring ProcessManager::GetProcessList(bool only1c)
{
	return MB2WC(ProcessList(only1c).Enumerate());
}

std::wstring ProcessManager::GetProcessInfo(int64_t pid)
{
	return ProcessInfo((unsigned long)pid);
}

std::wstring ProcessManager::FindProcess(const std::wstring& name)
{
	return {};
}

void ProcessManager::Sleep(int64_t ms)
{
	::usleep((unsigned long)ms * 1000);
}

bool ProcessManager::ConsoleOut(const std::wstring& text, int64_t encoding)
{
	std::wcout << text;
	return true;
}

#endif //_WINDOWS

#ifdef USE_BOOST

#include <boost/filesystem.hpp>

std::string ProcessManager::GetFreeDiskSpace(const std::wstring& disk)
{
	try {
		boost::filesystem::path path(disk);
		auto space = boost::filesystem::space(path);
		JSON json;
		json["available"] = space.available;
		json["capacity"] = space.capacity;
		json["free"] = space.free;
		return json.dump();
	}
	catch (boost::filesystem::filesystem_error& e) {
		return JSON({ {"error", cp1251_to_utf8(e.what())} }).dump();
	}
	catch (std::exception& e) {
		return JSON({ {"error", e.what()} }).dump();
	}
}

#else //USE_BOOST

std::string ProcessManager::GetFreeDiskSpace(const std::wstring& disk)
{
#ifdef _WINDOWS
	ULARGE_INTEGER uFreeBytesAvailableToCaller;
	ULARGE_INTEGER uTotalNumberOfBytes;
	ULARGE_INTEGER uTotalNumberOfFreeBytes;
	BOOL ok = GetDiskFreeSpaceExW(disk.c_str(), &uFreeBytesAvailableToCaller, &uTotalNumberOfBytes, &uTotalNumberOfFreeBytes);
	if (ok) {
		JSON json{
			{"available", uFreeBytesAvailableToCaller.QuadPart},
			{"capacity", uTotalNumberOfBytes.QuadPart},
			{"free", uTotalNumberOfFreeBytes.QuadPart}
		};
		return json.dump();
	}
	else {
		return JSON({ {"error", GetLastError()} }).dump();
	}
#else //_WINDOWS
	return JSON({ {"error", "Boost not found"} }).dump();
#endif //_WINDOWS
}

#endif // USE_BOOST
