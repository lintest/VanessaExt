#include "stdafx.h"
#include "ProcessManager.h"
#include "ProcessUtils.h"

static void AddProcessStartTime(JSON& json, int64_t pid)
{
	std::string startTime;
	std::string startTimeUtc;
	if (ProcessUtils::GetStartTime(pid, startTime, startTimeUtc)) {
		json["StartTime"] = startTime;
		json["StartTimeUTC"] = startTimeUtc;
	}
	else {
		json["StartTime"] = "";
		json["StartTimeUTC"] = "";
	}
}

#ifdef _WINDOWS

#define _WIN32_DCOM
#define DPSAPI_VERSION 1

#include <iostream>
#include <sstream>
#include <comdef.h>
#include <psapi.h>
#include <wbemidl.h>

#pragma comment(lib, "psapi.lib")
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
							VariantInit(&vtDest);
							hr = VariantChangeType(&vtDest, &vtProp, VARIANT_ALPHABOOL, VT_BSTR);
							if SUCCEEDED(hr) json[WC2MB(name)] = WC2MB(vtDest.bstrVal);
							VariantClear(&vtDest);
						}
					}
				}
				VariantClear(&vtProp);
				SysFreeString(name);
			}
			SafeArrayDestroy(pNames);
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
	JSON json = ProcessEnumerator(query.c_str()).json();
	if (json.is_array()) {
		for (auto& item : json) {
			if (!item.is_object()) continue;
			auto pid = item.find("ProcessId");
			if (pid == item.end() || !pid->is_number_integer()) continue;
			AddProcessStartTime(item, pid->get<int64_t>());
		}
	}
	return MB2WC(json.dump());
}

typedef BOOL (STDAPICALLTYPE* PGetProcessMemoryInfo)(HANDLE Process, PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb);

std::wstring ProcessManager::GetProcessInfo(int64_t pid)
{
	std::wstring query;
	query.append(L"SELECT * FROM Win32_Process WHERE ProcessId=");
	query.append(std::to_wstring(pid));
	JSON json = ProcessEnumerator(query.c_str()).json();
	if (json.is_array() && json.size() == 1) {
		json = json[0];
		AddProcessStartTime(json, pid);
		PROCESS_MEMORY_COUNTERS_EX pmc;
		ZeroMemory(&pmc, sizeof(pmc));
		static HMODULE hLibKernel32 = nullptr;
		static PGetProcessMemoryInfo pGetProcessMemoryInfo = &GetProcessMemoryInfo;
		if (!hLibKernel32) {
			hLibKernel32 = LoadLibrary(L"kernel32.dll");
			if (hLibKernel32) {
				auto proc = reinterpret_cast<PGetProcessMemoryInfo>(
					GetProcAddress(hLibKernel32, "K32GetProcessMemoryInfo"));
				if (proc) pGetProcessMemoryInfo = proc;
			}
		}
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE, (DWORD)pid);
		if (hProcess) {
			BOOL ok = pGetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
			CloseHandle(hProcess);
			if (ok) {
				json["PageFaultCount"] = pmc.PageFaultCount;
				json["PeakWorkingSetSize"] = pmc.PeakWorkingSetSize;
				json["WorkingSetSize"] = pmc.WorkingSetSize;
				json["QuotaPeakPagedPoolUsage"] = pmc.QuotaPeakPagedPoolUsage;
				json["QuotaPagedPoolUsage"] = pmc.QuotaPagedPoolUsage;
				json["QuotaPeakNonPagedPoolUsage"] = pmc.QuotaPeakNonPagedPoolUsage;
				json["QuotaNonPagedPoolUsage"] = pmc.QuotaNonPagedPoolUsage;
				json["PagefileUsage"] = pmc.PagefileUsage;
				json["PeakPagefileUsage"] = pmc.PeakPagefileUsage;
				json["PrivateUsage"] = pmc.PrivateUsage;
			}
		}
		return MB2WC(json.dump());
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
	query.append(L" AND CommandLine LIKE '% -TPort%");
	query.append(std::to_wstring(port)).append(L"%'");
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
				&& (wcscmp(L"V8TopLevelFrameSDI", buffer) == 0 
					|| (wcscmp(L"V8TopLevelFrame", buffer) == 0)
					|| (GetWindowTextLength(hWnd) && wcsncmp(buffer, L"V8Window", 8) == 0))
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
		const int64_t foundPid = j["ProcessId"].get<int64_t>();
		AddProcessStartTime(j, foundPid);
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
#include <sys/statvfs.h>
#include <dirent.h>
#include <errno.h>
#include <regex>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <fstream>
#include <iterator>
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
		json["ProcessId"] = pid;
		AddProcessStartTime(json, pid);
		return false;
	}

public:
	ClientFinder(int port) : m_port(port) {}

private:

	std::vector<std::string> getCommandLineArguments(int pid) {
		std::vector<std::string> arguments;
		std::string path = "/proc/" + std::to_string(pid) + "/cmdline";
		std::ifstream file(path, std::ios::binary);
		if (file.is_open()) {
			std::string arg;
			while (std::getline(file, arg, '\0')) {
				if (!arg.empty()) {
					arguments.push_back(arg);
				}
			}
		}
		return arguments;
	}

	bool NotFound(unsigned long pid) {
		auto arguments = getCommandLineArguments(pid);
		std::string portParam = "-TPort";
		std::string portValue = to_string(m_port);
		bool portNotFound = true;
		for (const auto& arg : arguments) {
			if (portNotFound) {
				portNotFound = (arg != portParam);
			} else {
				return (arg != portValue);
			}
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
		j["ProcessId"] = pid;
		AddProcessStartTime(j, pid);
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
		auto name = GetProcessName(pid);
		if (!name.empty()) {
			json["Name"] = name;
			json["ProcessId"] = pid;
			json["CommandLine"] = GetCommandLine(pid);
			AddProcessStartTime(json, pid);
		}
	}
	bool empty() {
		return json.empty();
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
	ProcessInfo pi{(unsigned long)pid};
	if (pi.empty())
		return {};

	return pi;
}

namespace {

bool IsProcessId(const char* value)
{
	if (!value || !*value) return false;
	for (const char* p = value; *p; ++p) {
		if (*p < '0' || *p > '9') return false;
	}
	return true;
}

std::string ReadProcessFile(const std::string& path, bool firstLineOnly)
{
	std::ifstream file(path, std::ios::binary);
	if (!file) return {};
	std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	if (firstLineOnly) {
		auto pos = text.find('\n');
		if (pos != std::string::npos) text.resize(pos);
	}
	std::replace(text.begin(), text.end(), '\0', ' ');
	return text;
}

std::string TrimProcessQuery(const std::string& value)
{
	size_t begin = value.find_first_not_of(" \t\r\n");
	if (begin == std::string::npos) return {};
	size_t end = value.find_last_not_of(" \t\r\n");
	return value.substr(begin, end - begin + 1);
}

std::string ToLower(const std::string& value)
{
	std::string result = value;
	std::transform(result.begin(), result.end(), result.begin(),
		[](unsigned char c) { return (char)::tolower(c); });
	return result;
}

// Converts a SQL LIKE pattern (or a file mask with * and ?) to a regular expression.
std::string MaskToRegex(const std::string& mask)
{
	std::string result;
	for (char c : mask) {
		switch (c) {
		case '%':
		case '*': result += ".*"; break;
		case '_':
		case '?': result += "."; break;
		default:
			if (std::strchr("\\^$.|+()[]{}", c)) result += '\\';
			result += c;
			break;
		}
	}
	return result;
}

std::regex MakeMaskRegex(const std::string& mask)
{
	return std::regex(MaskToRegex(mask), std::regex::icase | std::regex::optimize);
}

struct ProcessFilter
{
	bool matchAny = false;
	bool byName = false;
	bool byCommandLine = false;
	std::regex name;
	std::regex commandLine;
};

// Understands a bare mask (matched against the name or the command line) and a
// subset of WQL: SELECT ... FROM Win32_Process WHERE Name = '...' AND CommandLine LIKE '%...%'.
bool ParseProcessQuery(const std::string& query, ProcessFilter& filter)
{
	std::string lower = ToLower(query);
	size_t where = lower.find(" where ");
	if (where == std::string::npos) return false;

	std::string conditions = query.substr(where + 7);
	std::regex condition(R"((\w+)\s*(=|like)\s*'([^']*)')", std::regex::icase);
	for (auto it = std::sregex_iterator(conditions.begin(), conditions.end(), condition);
		it != std::sregex_iterator(); ++it) {
		std::string field = ToLower((*it)[1].str());
		std::string value = (*it)[3].str();
		if (field == "name") {
			filter.byName = true;
			filter.name = MakeMaskRegex(value);
		}
		else if (field == "commandline") {
			filter.byCommandLine = true;
			filter.commandLine = MakeMaskRegex(value);
		}
	}
	return filter.byName || filter.byCommandLine;
}

} // namespace

std::wstring ProcessManager::FindProcess(const std::wstring& query)
{
	std::string text = TrimProcessQuery(WC2MB(query));
	if (text.empty()) return {};

	ProcessFilter filter;
	try {
		if (!ParseProcessQuery(text, filter)) {
			// Bare mask: match either the process name or its command line.
			filter.matchAny = true;
			filter.byName = true;
			filter.byCommandLine = true;
			filter.name = MakeMaskRegex(text);
			filter.commandLine = filter.name;
		}
	}
	catch (const std::regex_error&) {
		return {};
	}

	JSON result = JSON::array();
	DIR* dir = opendir("/proc");
	if (!dir) return MB2WC(result.dump());

	while (auto entry = readdir(dir)) {
		if (!IsProcessId(entry->d_name)) continue;

		unsigned long pid = strtoul(entry->d_name, nullptr, 10);
		std::string path = std::string("/proc/") + entry->d_name;
		std::string name = ReadProcessFile(path + "/comm", true);
		if (name.empty()) continue;
		std::string commandLine = ReadProcessFile(path + "/cmdline", false);

		bool nameMatches = !filter.byName || std::regex_search(name, filter.name);
		bool commandLineMatches = !filter.byCommandLine || std::regex_search(commandLine, filter.commandLine);
		bool match = filter.matchAny
			? ((filter.byName && nameMatches) || (filter.byCommandLine && commandLineMatches))
			: (nameMatches && commandLineMatches);
		if (!match) continue;

		JSON json;
		json["Name"] = name;
		json["ProcessId"] = pid;
		json["CommandLine"] = commandLine;
		AddProcessStartTime(json, pid);
		result.push_back(json);
	}
	closedir(dir);

	return MB2WC(result.dump());
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

std::wstring ProcessManager::StartProcess(const std::wstring& command,
	const std::wstring& windowMode)
{
	return MB2WC(ProcessUtils::StartProcess(command, windowMode));
}

std::wstring ProcessManager::StopProcess(int64_t pid,
	const std::wstring& startTime, bool force)
{
	return MB2WC(ProcessUtils::StopProcess(pid, startTime, force));
}

std::wstring ProcessManager::GetProcessState(int64_t pid,
	const std::wstring& startTime)
{
	return MB2WC(ProcessUtils::GetProcessState(pid, startTime));
}

#ifdef USE_BOOST
#include <boost/filesystem.hpp>
#endif //USE_BOOST

std::string ProcessManager::GetFreeDiskSpace(const std::wstring& disk)
{
#ifdef _WINDOWS
#ifdef USE_BOOST
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
#else
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
#endif //USE_BOOST
#else //_WINDOWS
	std::string path = WC2MB(disk);
	if (path.empty()) path = "/";

	struct statvfs info;
	if (statvfs(path.c_str(), &info) != 0) {
		return JSON({ {"error", errno} }).dump();
	}

	uint64_t blockSize = (uint64_t)info.f_frsize;
	JSON json{
		{"available", blockSize * (uint64_t)info.f_bavail},
		{"capacity", blockSize * (uint64_t)info.f_blocks},
		{"free", blockSize * (uint64_t)info.f_bfree}
	};
	return json.dump();
#endif //_WINDOWS
}
