#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "stdafx.h"
#include "ProcessUtils.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <climits>
#include <codecvt>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <locale>
#include <map>
#include <mutex>
#include <sstream>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WINDOWS
#ifndef WC_ERR_INVALID_CHARS
#define WC_ERR_INVALID_CHARS 0x00000080
#endif
#else
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#ifndef SYS_pidfd_open
#if defined(__x86_64__) || defined(__aarch64__) || defined(__riscv)
#define SYS_pidfd_open 434
#define SYS_pidfd_send_signal 424
#endif
#endif
extern char** environ;
#endif

namespace {

constexpr int kErrorOk = 0;
constexpr int kErrorArgument = 1;
constexpr int kErrorStart = 2;
constexpr int kErrorNotFound = 3;
constexpr int kErrorStartTimeMismatch = 4;
constexpr int kErrorAccessDenied = 5;
constexpr int kErrorInternal = 7;
constexpr int kErrorStartTime = 8;
constexpr int kErrorImmediateExit = 9;

enum class WindowMode {
	Normal,
	Maximized,
	Hidden
};

bool WideToAscii(const std::wstring& value, std::string& result)
{
	result.clear();
	result.reserve(value.size());
	for (wchar_t ch : value) {
		const unsigned long code = static_cast<unsigned long>(ch);
		if (code > 0x7f) return false;
		result.push_back(static_cast<char>(ch));
	}
	return true;
}

std::string Trim(const std::string& value)
{
	size_t first = 0;
	while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) ++first;
	size_t last = value.size();
	while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) --last;
	return value.substr(first, last - first);
}

bool ParseWindowMode(const std::wstring& value, WindowMode& mode)
{
	std::string text;
	if (!WideToAscii(value, text)) return false;
	std::transform(text.begin(), text.end(), text.begin(),
		[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	if (text == "normal") {
		mode = WindowMode::Normal;
		return true;
	}
	if (text == "maximized") {
		mode = WindowMode::Maximized;
		return true;
	}
	if (text == "hidden") {
		mode = WindowMode::Hidden;
		return true;
	}
	return false;
}

bool ParseStartTime(const std::wstring& value, std::string& canonical)
{
	canonical.clear();
	if (!WideToAscii(value, canonical)) return false;
	if (canonical.empty() || canonical.size() > 20) return false;
	if (canonical.size() > 1 && canonical[0] == '0') return false;

	uint64_t result = 0;
	for (char ch : canonical) {
		if (ch < '0' || ch > '9') return false;
		const uint64_t digit = static_cast<uint64_t>(ch - '0');
		if (result > (UINT64_MAX - digit) / 10) return false;
		result = result * 10 + digit;
	}
	return result != 0;
}

bool ValidPid(int64_t pid)
{
#ifdef _WINDOWS
	return pid > 0 && pid <= static_cast<int64_t>(MAXDWORD);
#else
	return pid > 0 && pid <= static_cast<int64_t>(INT_MAX);
#endif
}

std::string StartJsonError(int code, const std::string& message, int64_t pid = 0,
	const std::string& startTime = std::string(), const char* state = "Unknown",
	bool hasExitCode = false, int exitCode = 0)
{
	JSON json;
	json["ok"] = false;
	json["ProcessId"] = pid;
	json["StartTime"] = startTime;
	json["State"] = state;
	if (hasExitCode) json["ExitCode"] = exitCode;
	else json["ExitCode"] = nullptr;
	json["error"] = message;
	json["errorCode"] = code;
	return json.dump();
}

std::string StartJsonSuccess(int64_t pid, const std::string& startTime,
	const std::string& startTimeUtc, int64_t processGroupId)
{
	JSON json;
	json["ok"] = true;
	json["ProcessId"] = pid;
	json["StartTime"] = startTime;
	json["StartTimeUTC"] = startTimeUtc;
	json["State"] = "Running";
	json["ProcessGroupId"] = processGroupId;
	json["error"] = "";
	json["errorCode"] = kErrorOk;
	return json.dump();
}

std::string StateJson(const char* state, bool startTimeMatches, bool hasExitCode,
	int exitCode, int errorCode, const std::string& error)
{
	JSON json;
	json["State"] = state;
	json["StartTimeMatches"] = startTimeMatches;
	if (hasExitCode) json["ExitCode"] = exitCode;
	else json["ExitCode"] = nullptr;
	json["error"] = error;
	json["errorCode"] = errorCode;
	return json.dump();
}

std::string StopJsonSuccess(bool alreadyExited, bool hasExitCode, int exitCode,
	const char* state)
{
	JSON json;
	json["ok"] = true;
	json["AlreadyExited"] = alreadyExited;
	if (hasExitCode) json["ExitCode"] = exitCode;
	else json["ExitCode"] = nullptr;
	json["State"] = state;
	json["error"] = "";
	json["errorCode"] = kErrorOk;
	return json.dump();
}

std::string StopJsonError(int code, const std::string& message, const char* state)
{
	JSON json;
	json["ok"] = false;
	json["AlreadyExited"] = false;
	json["ExitCode"] = nullptr;
	json["State"] = state;
	json["error"] = message;
	json["errorCode"] = code;
	return json.dump();
}

std::atomic<int> g_refs{ 0 };

// Shutdown() ждёт завершения уже начатых вызовов, чтобы не закрыть handle
// или не остановить reaper во время работы другого потока.
std::mutex g_lifecycleMutex;
int g_activeCalls = 0;
bool g_shuttingDown = false;

// Короткая пауза для handshake при выгрузке компоненты.
// std::condition_variable здесь не используется намеренно: не-inline
// wait(unique_lock<mutex>&) в libstdc++ из GCC 12 помечен версией
// GLIBCXX_3.4.30, и .so не загружается на Ubuntu 18.04 (GLIBCXX_3.4.22)
// и Ubuntu 20.04 (GLIBCXX_3.4.28).
void WaitMilliseconds(int milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

class CallGuard
{
public:
	CallGuard()
	{
		std::unique_lock<std::mutex> lock(g_lifecycleMutex);
		while (g_shuttingDown) {
			lock.unlock();
			WaitMilliseconds(5);
			lock.lock();
		}
		++g_activeCalls;
	}

	~CallGuard()
	{
		std::lock_guard<std::mutex> lock(g_lifecycleMutex);
		--g_activeCalls;
	}
};

// Ждёт завершения уже начатых вызовов; вызывается только из Shutdown().
void WaitActiveCallsFinished()
{
	std::unique_lock<std::mutex> lock(g_lifecycleMutex);
	g_shuttingDown = true;
	while (g_activeCalls != 0) {
		lock.unlock();
		WaitMilliseconds(5);
		lock.lock();
	}
}

#ifdef _WINDOWS

struct Entry {
	HANDLE process = nullptr;
	std::string startTime;
	bool hasStartTime = false;
	bool exited = false;
	bool hasExitCode = false;
	DWORD exitCode = 0;
};

std::mutex g_mutex;
std::map<DWORD, Entry> g_entries;

std::string WideToUtf8(const std::wstring& value)
{
	if (value.empty()) return {};
	int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
		value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
	if (size <= 0) {
		size = WideCharToMultiByte(CP_UTF8, 0,
			value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
	}
	if (size <= 0) return {};
	std::string result(static_cast<size_t>(size), '\0');
	if (WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
		result.data(), size, nullptr, nullptr) <= 0) {
		return {};
	}
	return result;
}

std::string LastErrorText(DWORD error)
{
	LPWSTR buffer = nullptr;
	const DWORD length = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, error, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
	if (length == 0 || !buffer) {
		return "Win32 error " + std::to_string(error);
	}
	std::wstring text(buffer, length);
	LocalFree(buffer);
	while (!text.empty() && (text.back() == L'\r' || text.back() == L'\n' ||
		text.back() == L' ' || text.back() == L'.')) {
		text.pop_back();
	}
	return WideToUtf8(text);
}

std::string LastErrorText()
{
	return LastErrorText(GetLastError());
}

uint64_t FileTimeValue(const FILETIME& value)
{
	ULARGE_INTEGER result;
	result.LowPart = value.dwLowDateTime;
	result.HighPart = value.dwHighDateTime;
	return result.QuadPart;
}

std::string FileTimeUtc(const FILETIME& value)
{
	SYSTEMTIME systemTime;
	if (!FileTimeToSystemTime(&value, &systemTime)) return {};
	const unsigned milliseconds = static_cast<unsigned>(
		(FileTimeValue(value) % 10000000ULL) / 10000ULL);
	char buffer[40];
	std::snprintf(buffer, sizeof(buffer), "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
		systemTime.wYear, systemTime.wMonth, systemTime.wDay,
		systemTime.wHour, systemTime.wMinute, systemTime.wSecond,
		milliseconds);
	return buffer;
}

bool ReadStartTime(HANDLE process, std::string& startTime, std::string& startTimeUtc)
{
	FILETIME creation, exit, kernel, user;
	if (!GetProcessTimes(process, &creation, &exit, &kernel, &user)) return false;
	startTime = std::to_string(FileTimeValue(creation));
	startTimeUtc = FileTimeUtc(creation);
	return true;
}

void CloseEntry(Entry& entry)
{
	if (entry.process) {
		CloseHandle(entry.process);
		entry.process = nullptr;
	}
}

void ReplaceEntry(DWORD pid, HANDLE process, const std::string& startTime)
{
	auto it = g_entries.find(pid);
	if (it != g_entries.end()) {
		CloseEntry(it->second);
		g_entries.erase(it);
	}
	Entry entry;
	entry.process = process;
	entry.startTime = startTime;
	entry.hasStartTime = true;
	entry.exited = false;
	entry.hasExitCode = false;
	g_entries[pid] = entry;
}

struct FindWindowContext {
	DWORD pid = 0;
	HWND window = nullptr;
};

BOOL CALLBACK FindMainWindowCallback(HWND window, LPARAM parameter)
{
	auto context = reinterpret_cast<FindWindowContext*>(parameter);
	DWORD pid = 0;
	GetWindowThreadProcessId(window, &pid);
	if (pid != context->pid) return TRUE;
	if (!IsWindowVisible(window)) return TRUE;
	if (GetWindow(window, GW_OWNER) != nullptr) return TRUE;
	context->window = window;
	return FALSE;
}

HWND FindMainWindow(DWORD pid)
{
	FindWindowContext context;
	context.pid = pid;
	EnumWindows(FindMainWindowCallback, reinterpret_cast<LPARAM>(&context));
	return context.window;
}

enum class WaitResult {
	Exited,
	Running,
	Unknown
};

WaitResult WaitExited(HANDLE process, DWORD& exitCode, bool& hasExitCode)
{
	const DWORD result = WaitForSingleObject(process, 0);
	if (result == WAIT_TIMEOUT) return WaitResult::Running;
	if (result != WAIT_OBJECT_0) return WaitResult::Unknown;
	hasExitCode = GetExitCodeProcess(process, &exitCode) != FALSE;
	if (!hasExitCode) exitCode = 0;
	return WaitResult::Exited;
}

#else

struct Entry {
	pid_t pid = 0;
	std::string startTime;
	bool hasStartTime = false;
	bool exited = false;
	bool hasExitCode = false;
	int exitCode = 0;
};

std::mutex g_mutex;
std::map<pid_t, Entry> g_entries;
pthread_t g_reaperThread;
std::atomic<bool> g_stop{ false };
bool g_reaperStarted = false;

std::string WideToUtf8(const std::wstring& value)
{
	try {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(value);
	}
	catch (...) {
		return {};
	}
}

std::string ErrnoText(int error)
{
	return std::generic_category().message(error);
}

struct ProcStat {
	char state = 0;
	uint64_t startTime = 0;
};

enum class ProcResult {
	Ok,
	NotFound,
	AccessDenied,
	Error
};

ProcResult ReadProcStat(pid_t pid, ProcStat& result)
{
	char path[64];
	std::snprintf(path, sizeof(path), "/proc/%d/stat", static_cast<int>(pid));
	int file = open(path, O_RDONLY | O_CLOEXEC);
	if (file < 0) {
		if (errno == ENOENT || errno == ESRCH) return ProcResult::NotFound;
		if (errno == EACCES || errno == EPERM) return ProcResult::AccessDenied;
		return ProcResult::Error;
	}

	std::string data;
	char buffer[4096];
	ssize_t readSize = 0;
	while ((readSize = read(file, buffer, sizeof(buffer))) > 0) {
		data.append(buffer, static_cast<size_t>(readSize));
		if (data.size() > 65536) break;
	}
	const int readError = errno;
	close(file);
	if (readSize < 0) {
		if (readError == ENOENT || readError == ESRCH) return ProcResult::NotFound;
		if (readError == EACCES || readError == EPERM) return ProcResult::AccessDenied;
		return ProcResult::Error;
	}

	const size_t close = data.rfind(')');
	if (close == std::string::npos) return ProcResult::Error;
	std::istringstream stream(data.substr(close + 1));
	std::string state;
	if (!(stream >> state) || state.empty()) return ProcResult::Error;
	result.state = state[0];
	result.startTime = 0;

	std::string token;
	for (int index = 1; index <= 19; ++index) {
		if (!(stream >> token)) return ProcResult::Error;
		if (index != 19) continue;
		errno = 0;
		char* end = nullptr;
		const unsigned long long value = std::strtoull(token.c_str(), &end, 10);
		if (errno != 0 || end == token.c_str() || *end != '\0') return ProcResult::Error;
		result.startTime = static_cast<uint64_t>(value);
	}
	return ProcResult::Ok;
}

std::string ImmediateExitJson(int64_t pid, const std::string& startTime,
	bool hasStartTime, bool hasExitCode, int exitCode)
{
	if (!hasStartTime) {
		return StartJsonError(kErrorStartTime,
			"Process exited before StartTime could be read", pid, std::string(),
			"Unknown", hasExitCode, exitCode);
	}
	return StartJsonError(kErrorImmediateExit,
		"Process exited immediately after start", pid, startTime, "Exited",
		hasExitCode, exitCode);
}

uint64_t ReadBootTime()
{
	std::ifstream file("/proc/stat");
	if (!file) return 0;
	std::string line;
	while (std::getline(file, line)) {
		if (line.compare(0, 6, "btime ") != 0) continue;
		std::istringstream stream(line.substr(6));
		uint64_t value = 0;
		stream >> value;
		return value;
	}
	return 0;
}

std::string FormatStartTimeUtc(uint64_t startTicks)
{
	long ticksPerSecond = sysconf(_SC_CLK_TCK);
	if (ticksPerSecond <= 0) ticksPerSecond = 100;
	const uint64_t bootTime = ReadBootTime();
	if (bootTime == 0) return {};

	const time_t seconds = static_cast<time_t>(bootTime + startTicks / static_cast<uint64_t>(ticksPerSecond));
	const unsigned milliseconds = static_cast<unsigned>(
		(startTicks % static_cast<uint64_t>(ticksPerSecond)) * 1000 /
		static_cast<uint64_t>(ticksPerSecond));
	struct tm timeValue;
	if (!gmtime_r(&seconds, &timeValue)) return {};

	// Запас под worst case GCC: для %04d он закладывает до 11 символов
	// (знак + 10 цифр), для %03u — до 10, суммарно 84. Фактическая длина
	// всегда 24 символа + терминатор: год 4 цифры, остальные поля по 2,
	// миллисекунды 0..999.
	char buffer[96];
	std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03uZ",
		timeValue.tm_year + 1900, timeValue.tm_mon + 1, timeValue.tm_mday,
		timeValue.tm_hour, timeValue.tm_min, timeValue.tm_sec, milliseconds);
	return buffer;
}

// Период опроса waitpid. Отдельный поток нужен, чтобы zombie-процессы
// собирались без участия 1С. Ожидание — nanosleep, без condition_variable
// и chrono-примитивов: на новых libstdc++ они тянут версии символов
// GLIBCXX_3.4.30/GLIBC_2.30 и ломают загрузку на старых Ubuntu.
const long kReaperTickNanoseconds = 100 * 1000 * 1000L;

void* ReaperLoop(void*)
{
	while (!g_stop.load()) {
		struct timespec remaining = { 0, kReaperTickNanoseconds };
		while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR) {}
		if (g_stop.load()) break;

		std::lock_guard<std::mutex> lock(g_mutex);
		for (auto& item : g_entries) {
			Entry& entry = item.second;
			if (entry.exited) continue;
			int status = 0;
			const pid_t result = waitpid(entry.pid, &status, WNOHANG);
			if (result == entry.pid) {
				entry.exited = true;
				if (WIFEXITED(status)) {
					entry.hasExitCode = true;
					entry.exitCode = WEXITSTATUS(status);
				}
				else if (WIFSIGNALED(status)) {
					entry.hasExitCode = true;
					entry.exitCode = 128 + WTERMSIG(status);
				}
			}
			else if (result == -1 && errno == ECHILD) {
				// Процесс уже собран (например, SIGCHLD=SIG_IGN).
				entry.exited = true;
				entry.hasExitCode = false;
			}
		}
	}
	return nullptr;
}

bool EnsureReaper()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if (g_reaperStarted) return true;
	static bool exitHandlerRegistered = false;
	if (!exitHandlerRegistered) {
		std::atexit([] { ProcessUtils::Shutdown(); });
		exitHandlerRegistered = true;
	}
	g_stop.store(false);
	if (pthread_create(&g_reaperThread, nullptr, ReaperLoop, nullptr) != 0) {
		g_stop.store(true);
		return false;
	}
	g_reaperStarted = true;
	return true;
}

bool SplitCommandLine(const std::string& command,
	std::vector<std::string>& arguments, std::string& error)
{
	arguments.clear();
	error.clear();
	std::string current;
	bool tokenStarted = false;
	bool singleQuoted = false;
	bool doubleQuoted = false;

	for (size_t index = 0; index < command.size(); ++index) {
		const char ch = command[index];
		if (singleQuoted) {
			tokenStarted = true;
			if (ch == '\'') singleQuoted = false;
			else current.push_back(ch);
			continue;
		}
		if (doubleQuoted) {
			tokenStarted = true;
			if (ch == '"') {
				doubleQuoted = false;
				continue;
			}
			// Shell отсутствует, поэтому backslash — обычный символ.
			current.push_back(ch);
			continue;
		}
		if (ch == '"') {
			doubleQuoted = true;
			tokenStarted = true;
			continue;
		}
		if (ch == '\'') {
			singleQuoted = true;
			tokenStarted = true;
			continue;
		}
		if (std::isspace(static_cast<unsigned char>(ch))) {
			if (tokenStarted) {
				arguments.push_back(current);
				current.clear();
				tokenStarted = false;
			}
			continue;
		}
		current.push_back(ch);
		tokenStarted = true;
	}

	if (singleQuoted || doubleQuoted) {
		error = "Unterminated quote in command";
		return false;
	}
	if (tokenStarted) arguments.push_back(current);
	if (arguments.empty() || arguments[0].empty()) {
		error = "Empty executable";
		return false;
	}
	return true;
}

int OpenPidFd(pid_t pid)
{
#if defined(SYS_pidfd_open)
	return static_cast<int>(syscall(SYS_pidfd_open, pid, 0));
#else
	(void)pid;
	errno = ENOSYS;
	return -1;
#endif
}

int SendPidFdSignal(int fd, int signal)
{
#if defined(SYS_pidfd_send_signal)
	return static_cast<int>(syscall(SYS_pidfd_send_signal, fd, signal, nullptr, 0));
#else
	(void)fd;
	(void)signal;
	errno = ENOSYS;
	return -1;
#endif
}

bool AddCloseFileActions(posix_spawn_file_actions_t& actions)
{
	// Не вызываем addclosefrom_np напрямую: на glibc 2.34+ это добавит
	// зависимость GLIBC_2.34 и не даст загрузить .so на CentOS 7.
	DIR* directory = opendir("/proc/self/fd");
	if (directory) {
		const int directoryFd = dirfd(directory);
		struct dirent* item = nullptr;
		while ((item = readdir(directory)) != nullptr) {
			if (item->d_name[0] == '.') continue;
			const int fd = std::atoi(item->d_name);
			if (fd <= 2 || fd == directoryFd) continue;
			posix_spawn_file_actions_addclose(&actions, fd);
		}
		closedir(directory);
		return true;
	}

	long limit = sysconf(_SC_OPEN_MAX);
	if (limit <= 0 || limit > 4096) limit = 4096;
	for (int fd = 3; fd < limit; ++fd) {
		if (fcntl(fd, F_GETFD) == -1) continue;
		posix_spawn_file_actions_addclose(&actions, fd);
	}
	return true;
}

#endif

} // namespace

#ifdef _WINDOWS

std::string ProcessUtils::StartProcess(const std::wstring& command,
	const std::wstring& windowMode)
{
	CallGuard callGuard;

	WindowMode mode;
	if (!ParseWindowMode(windowMode, mode)) {
		return StartJsonError(kErrorArgument, "Unknown window mode");
	}
	if (Trim(WideToUtf8(command)).empty()) {
		return StartJsonError(kErrorArgument, "Empty command");
	}

	std::vector<wchar_t> commandLine(command.begin(), command.end());
	commandLine.push_back(L'\0');

	STARTUPINFOW startup = {};
	startup.cb = sizeof(startup);
	if (mode != WindowMode::Normal) {
		startup.dwFlags = STARTF_USESHOWWINDOW;
		switch (mode) {
		case WindowMode::Maximized:
			startup.wShowWindow = SW_SHOWMAXIMIZED;
			break;
		case WindowMode::Hidden:
			startup.wShowWindow = SW_HIDE;
			break;
		default:
			startup.wShowWindow = SW_SHOWNORMAL;
			break;
		}
	}

	DWORD creationFlags = CREATE_NEW_PROCESS_GROUP;
	if (mode == WindowMode::Hidden) creationFlags |= CREATE_NO_WINDOW;

	PROCESS_INFORMATION process = {};
	if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE,
		creationFlags, nullptr, nullptr, &startup, &process)) {
		return StartJsonError(kErrorStart,
			"CreateProcessW failed: " + LastErrorText());
	}
	CloseHandle(process.hThread);

	std::string startTime;
	std::string startTimeUtc;
	if (!ReadStartTime(process.hProcess, startTime, startTimeUtc)) {
		const std::string error = "GetProcessTimes failed: " + LastErrorText();
		::TerminateProcess(process.hProcess, 1);
		CloseHandle(process.hProcess);
		return StartJsonError(kErrorStartTime, error, process.dwProcessId);
	}

	const DWORD pid = process.dwProcessId;
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		ReplaceEntry(pid, process.hProcess, startTime);
	}

	const DWORD waitResult = WaitForSingleObject(process.hProcess, 200);
	if (waitResult == WAIT_FAILED) {
		return StartJsonError(kErrorInternal,
			"WaitForSingleObject failed: " + LastErrorText(),
			pid, startTime, "Unknown");
	}
	if (waitResult == WAIT_OBJECT_0) {
		DWORD exitCode = 0;
		const bool hasExitCode = GetExitCodeProcess(process.hProcess, &exitCode) != FALSE;
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			auto it = g_entries.find(pid);
			if (it != g_entries.end()) {
				it->second.exited = true;
				it->second.hasExitCode = hasExitCode;
				it->second.exitCode = exitCode;
			}
		}
		return StartJsonError(kErrorImmediateExit,
			"Process exited immediately after start", pid, startTime,
			"Exited", hasExitCode, static_cast<int>(exitCode));
	}

	return StartJsonSuccess(pid, startTime, startTimeUtc, 0);
}

std::string ProcessUtils::GetProcessState(int64_t pid, const std::wstring& startTime)
{
	CallGuard callGuard;

	std::string requested;
	if (!ValidPid(pid) || !ParseStartTime(startTime, requested)) {
		return StateJson("Unknown", false, false, 0, kErrorArgument,
			"Invalid PID or StartTime");
	}

	{
		std::lock_guard<std::mutex> lock(g_mutex);
		auto it = g_entries.find(static_cast<DWORD>(pid));
		if (it != g_entries.end()) {
			Entry& entry = it->second;
			if (!entry.hasStartTime || entry.startTime != requested) {
				return StateJson("StartTimeMismatch", false, false, 0,
					kErrorStartTimeMismatch, "StartTime mismatch");
			}
			if (entry.exited) {
				return StateJson("Exited", true, entry.hasExitCode,
					entry.exitCode, kErrorOk, "");
			}
			if (!entry.process) {
				return StateJson("Unknown", false, false, 0,
					kErrorInternal, "Registry handle is closed");
			}
			DWORD exitCode = 0;
			bool hasExitCode = false;
			const WaitResult waitResult = WaitExited(entry.process, exitCode, hasExitCode);
			if (waitResult == WaitResult::Exited) {
				entry.exited = true;
				entry.hasExitCode = hasExitCode;
				entry.exitCode = exitCode;
				return StateJson("Exited", true, hasExitCode,
					static_cast<int>(exitCode), kErrorOk, "");
			}
			if (waitResult == WaitResult::Unknown) {
				return StateJson("Unknown", true, false, 0,
					kErrorInternal, "WaitForSingleObject failed");
			}
			return StateJson("Running", true, false, 0, kErrorOk, "");
		}
	}

	HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
		FALSE, static_cast<DWORD>(pid));
	if (!process) {
		const DWORD error = GetLastError();
		if (error == ERROR_INVALID_PARAMETER) {
			return StateJson("NotFound", false, false, 0, kErrorOk, "");
		}
		if (error == ERROR_ACCESS_DENIED) {
			return StateJson("AccessDenied", false, false, 0,
				kErrorAccessDenied, "OpenProcess: access denied");
		}
		return StateJson("Unknown", false, false, 0, kErrorInternal,
			"OpenProcess failed: " + LastErrorText(error));
	}

	std::string currentStartTime;
	std::string currentStartTimeUtc;
	if (!ReadStartTime(process, currentStartTime, currentStartTimeUtc)) {
		const DWORD error = GetLastError();
		CloseHandle(process);
		if (error == ERROR_ACCESS_DENIED) {
			return StateJson("AccessDenied", false, false, 0,
				kErrorAccessDenied, "GetProcessTimes: access denied");
		}
		return StateJson("Unknown", false, false, 0, kErrorStartTime,
			"GetProcessTimes failed");
	}
	if (currentStartTime != requested) {
		CloseHandle(process);
		return StateJson("StartTimeMismatch", false, false, 0,
			kErrorStartTimeMismatch, "StartTime mismatch");
	}

	DWORD exitCode = 0;
	bool hasExitCode = false;
	const WaitResult waitResult = WaitExited(process, exitCode, hasExitCode);
	CloseHandle(process);
	if (waitResult == WaitResult::Exited) {
		return StateJson("Exited", true, hasExitCode, static_cast<int>(exitCode),
			kErrorOk, "");
	}
	if (waitResult == WaitResult::Unknown) {
		return StateJson("Unknown", true, false, 0,
			kErrorInternal, "WaitForSingleObject failed");
	}
	return StateJson("Running", true, false, 0, kErrorOk, "");
}

std::string ProcessUtils::StopProcess(int64_t pid, const std::wstring& startTime,
	bool force)
{
	CallGuard callGuard;

	std::string requested;
	if (!ValidPid(pid) || !ParseStartTime(startTime, requested)) {
		return StopJsonError(kErrorArgument, "Invalid PID or StartTime", "Unknown");
	}

	std::unique_lock<std::mutex> lock(g_mutex);
	HANDLE process = nullptr;
	bool localHandle = false;
	Entry* entry = nullptr;
	auto it = g_entries.find(static_cast<DWORD>(pid));
	if (it != g_entries.end()) {
		entry = &it->second;
		if (!entry->hasStartTime || entry->startTime != requested) {
			return StopJsonError(kErrorStartTimeMismatch,
				"StartTime mismatch", "StartTimeMismatch");
		}
		if (entry->exited) {
			return StopJsonSuccess(true, entry->hasExitCode, entry->exitCode, "Exited");
		}
		process = entry->process;
		if (!process) {
			return StopJsonError(kErrorInternal,
				"Registry handle is closed", "Unknown");
		}
	}

	if (!process) {
		lock.unlock();
		process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION |
			PROCESS_TERMINATE | SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
		if (!process) {
			const DWORD error = GetLastError();
			if (error == ERROR_INVALID_PARAMETER) {
				return StopJsonSuccess(true, false, 0, "NotFound");
			}
			if (error == ERROR_ACCESS_DENIED) {
				return StopJsonError(kErrorAccessDenied,
					"OpenProcess: access denied", "AccessDenied");
			}
			return StopJsonError(kErrorInternal,
				"OpenProcess failed: " + LastErrorText(error),
				"Unknown");
		}
		localHandle = true;

		std::string currentStartTime;
		std::string currentStartTimeUtc;
		if (!ReadStartTime(process, currentStartTime, currentStartTimeUtc)) {
			const DWORD error = GetLastError();
			CloseHandle(process);
			if (error == ERROR_ACCESS_DENIED) {
				return StopJsonError(kErrorAccessDenied,
					"GetProcessTimes: access denied", "AccessDenied");
			}
			return StopJsonError(kErrorStartTime,
				"GetProcessTimes failed", "Unknown");
		}
		if (currentStartTime != requested) {
			CloseHandle(process);
			return StopJsonError(kErrorStartTimeMismatch,
				"StartTime mismatch", "StartTimeMismatch");
		}
	}

	DWORD exitCode = 0;
	bool hasExitCode = false;
	const WaitResult waitResult = WaitExited(process, exitCode, hasExitCode);
	if (waitResult == WaitResult::Exited) {
		if (entry) {
			entry->exited = true;
			entry->hasExitCode = hasExitCode;
			entry->exitCode = exitCode;
		}
		if (localHandle) CloseHandle(process);
		return StopJsonSuccess(true, hasExitCode, static_cast<int>(exitCode), "Exited");
	}
	if (waitResult == WaitResult::Unknown) {
		if (localHandle) CloseHandle(process);
		return StopJsonError(kErrorInternal,
			"WaitForSingleObject failed", "Unknown");
	}

	if (force) {
		if (!::TerminateProcess(process, 1)) {
			const DWORD error = GetLastError();
			DWORD lateExitCode = 0;
			bool lateHasExitCode = false;
			const WaitResult lateWait = WaitExited(process, lateExitCode, lateHasExitCode);
			if (lateWait == WaitResult::Exited) {
				if (entry) {
					entry->exited = true;
					entry->hasExitCode = lateHasExitCode;
					entry->exitCode = lateExitCode;
				}
				if (localHandle) CloseHandle(process);
				return StopJsonSuccess(true, lateHasExitCode,
					static_cast<int>(lateExitCode), "Exited");
			}
			if (lateWait == WaitResult::Unknown) {
				if (localHandle) CloseHandle(process);
				return StopJsonError(kErrorInternal,
					"WaitForSingleObject failed", "Unknown");
			}
			if (localHandle) CloseHandle(process);
			if (error == ERROR_ACCESS_DENIED) {
				return StopJsonError(kErrorAccessDenied,
					"TerminateProcess: access denied", "AccessDenied");
			}
			return StopJsonError(kErrorInternal,
				"TerminateProcess failed: " + LastErrorText(error),
				"Running");
		}
	}
	else {
		const HWND window = FindMainWindow(static_cast<DWORD>(pid));
		if (!window) {
			DWORD lateExitCode = 0;
			bool lateHasExitCode = false;
			if (WaitExited(process, lateExitCode, lateHasExitCode) == WaitResult::Exited) {
				if (entry) {
					entry->exited = true;
					entry->hasExitCode = lateHasExitCode;
					entry->exitCode = lateExitCode;
				}
				if (localHandle) CloseHandle(process);
				return StopJsonSuccess(true, lateHasExitCode,
					static_cast<int>(lateExitCode), "Exited");
			}
			if (localHandle) CloseHandle(process);
			return StopJsonError(kErrorInternal,
				"Main window not found; use force termination", "Running");
		}
		DWORD lateExitCode = 0;
		bool lateHasExitCode = false;
		if (WaitExited(process, lateExitCode, lateHasExitCode) == WaitResult::Exited) {
			if (entry) {
				entry->exited = true;
				entry->hasExitCode = lateHasExitCode;
				entry->exitCode = lateExitCode;
			}
			if (localHandle) CloseHandle(process);
			return StopJsonSuccess(true, lateHasExitCode,
				static_cast<int>(lateExitCode), "Exited");
		}
		if (!PostMessageW(window, WM_CLOSE, 0, 0)) {
			if (localHandle) CloseHandle(process);
			return StopJsonError(kErrorInternal,
				"PostMessage(WM_CLOSE) failed", "Running");
		}
	}

	if (localHandle) CloseHandle(process);
	return StopJsonSuccess(false, false, 0, "Running");
}

bool ProcessUtils::GetStartTime(int64_t pid, std::string& startTime,
	std::string& startTimeUtc)
{
	CallGuard callGuard;

	startTime.clear();
	startTimeUtc.clear();
	if (!ValidPid(pid)) return false;

	HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
		static_cast<DWORD>(pid));
	if (!process) return false;
	const bool result = ReadStartTime(process, startTime, startTimeUtc);
	CloseHandle(process);
	return result;
}

void ProcessUtils::AddRef()
{
	g_refs.fetch_add(1);
}

void ProcessUtils::Release()
{
	const int references = g_refs.fetch_sub(1) - 1;
	if (references <= 0) Shutdown();
}

void ProcessUtils::Shutdown()
{
	WaitActiveCallsFinished();

	{
		std::lock_guard<std::mutex> lock(g_mutex);
		for (auto& item : g_entries) CloseEntry(item.second);
		g_entries.clear();
	}

	{
		std::lock_guard<std::mutex> lock(g_lifecycleMutex);
		g_shuttingDown = false;
	}
}

#else

std::string ProcessUtils::StartProcess(const std::wstring& command,
	const std::wstring& windowMode)
{
	CallGuard callGuard;

	WindowMode mode;
	if (!ParseWindowMode(windowMode, mode)) {
		return StartJsonError(kErrorArgument, "Unknown window mode");
	}

	const std::string commandLine = Trim(WideToUtf8(command));
	if (commandLine.empty()) {
		return StartJsonError(kErrorArgument, "Empty command");
	}

	std::vector<std::string> arguments;
	std::string parseError;
	if (!SplitCommandLine(commandLine, arguments, parseError)) {
		return StartJsonError(kErrorArgument, parseError);
	}

	(void)mode;
	if (!EnsureReaper()) {
		return StartJsonError(kErrorInternal,
			"Cannot start process reaper thread");
	}

	posix_spawnattr_t attributes;
	posix_spawn_file_actions_t actions;
	if (posix_spawnattr_init(&attributes) != 0) {
		return StartJsonError(kErrorInternal, "posix_spawnattr_init failed");
	}
	if (posix_spawn_file_actions_init(&actions) != 0) {
		posix_spawnattr_destroy(&attributes);
		return StartJsonError(kErrorInternal, "posix_spawn_file_actions_init failed");
	}

	short flags = POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF |
		POSIX_SPAWN_SETSIGMASK;
	sigset_t signalDefault;
	sigset_t signalMask;
	sigemptyset(&signalDefault);
	sigaddset(&signalDefault, SIGTERM);
	sigaddset(&signalDefault, SIGINT);
	sigaddset(&signalDefault, SIGHUP);
	sigaddset(&signalDefault, SIGCHLD);
	sigaddset(&signalDefault, SIGPIPE);
	sigemptyset(&signalMask);

	posix_spawnattr_setsigdefault(&attributes, &signalDefault);
	posix_spawnattr_setsigmask(&attributes, &signalMask);
	posix_spawnattr_setflags(&attributes, flags);
	posix_spawnattr_setpgroup(&attributes, 0);

	posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDWR, 0);
	posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_RDWR, 0);
	posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_RDWR, 0);
	AddCloseFileActions(actions);

	std::vector<char*> argv;
	argv.reserve(arguments.size() + 1);
	for (std::string& argument : arguments) argv.push_back(argument.data());
	argv.push_back(nullptr);

	pid_t pid = 0;
	const int spawnResult = posix_spawnp(&pid, arguments[0].c_str(),
		&actions, &attributes, argv.data(), environ);
	posix_spawn_file_actions_destroy(&actions);
	posix_spawnattr_destroy(&attributes);
	if (spawnResult != 0) {
		return StartJsonError(kErrorStart,
			"posix_spawnp failed: " + ErrnoText(spawnResult));
	}

	{
		std::lock_guard<std::mutex> lock(g_mutex);
		Entry entry;
		entry.pid = pid;
		g_entries[pid] = entry;
	}

	const auto deadline = std::chrono::steady_clock::now() +
		std::chrono::milliseconds(2000);
	bool startTimePrepared = false;
	std::string startTime;
	std::string startTimeUtc;

	while (std::chrono::steady_clock::now() < deadline) {
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			auto it = g_entries.find(pid);
			if (it != g_entries.end() && it->second.exited) {
				return ImmediateExitJson(pid, it->second.startTime,
					it->second.hasStartTime, it->second.hasExitCode,
					it->second.exitCode);
			}
		}

		ProcStat stat;
		const ProcResult statResult = ReadProcStat(pid, stat);
		if (statResult == ProcResult::NotFound) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
		if (statResult != ProcResult::Ok) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
		if (stat.startTime == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
		if (stat.state == 'Z' || stat.state == 'X' || stat.state == 'x') {
			for (int attempt = 0; attempt < 100; ++attempt) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				std::lock_guard<std::mutex> lock(g_mutex);
				auto it = g_entries.find(pid);
				if (it != g_entries.end() && it->second.exited) {
					return ImmediateExitJson(pid, it->second.startTime,
						it->second.hasStartTime, it->second.hasExitCode,
						it->second.exitCode);
				}
			}
			return ImmediateExitJson(pid, std::to_string(stat.startTime),
				true, false, 0);
		}

		startTime = std::to_string(stat.startTime);
		startTimeUtc = FormatStartTimeUtc(stat.startTime);
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			auto it = g_entries.find(pid);
			if (it != g_entries.end()) {
				it->second.hasStartTime = true;
				it->second.startTime = startTime;
			}
		}
		startTimePrepared = true;
		break;
	}

	if (!startTimePrepared) {
		// Не завершаем процесс по одному PID: StartTime не прочитан,
		// поэтому подтвердить идентичность невозможно.
		return StartJsonError(kErrorStartTime,
			"StartTime could not be read", pid, "", "Unknown");
	}

	// Команда могла успеть завершиться сразу после запуска.
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		auto it = g_entries.find(pid);
		if (it != g_entries.end() && it->second.exited) {
			return ImmediateExitJson(pid, it->second.startTime,
				it->second.hasStartTime, it->second.hasExitCode,
				it->second.exitCode);
		}
	}

	ProcStat finalStat;
	const ProcResult finalResult = ReadProcStat(pid, finalStat);
	if (finalResult == ProcResult::Ok) {
		const std::string current = std::to_string(finalStat.startTime);
		if (current != startTime) {
			return StartJsonError(kErrorStartTimeMismatch,
				"PID was reused: StartTime mismatch", pid, startTime,
				"StartTimeMismatch");
		}
		if (finalStat.state == 'Z' || finalStat.state == 'X' || finalStat.state == 'x') {
			for (int attempt = 0; attempt < 100; ++attempt) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				std::lock_guard<std::mutex> lock(g_mutex);
				auto it = g_entries.find(pid);
				if (it != g_entries.end() && it->second.exited) {
					return ImmediateExitJson(pid, it->second.startTime,
						it->second.hasStartTime, it->second.hasExitCode,
						it->second.exitCode);
				}
			}
			return ImmediateExitJson(pid, startTime, true, false, 0);
		}
	}
	else if (finalResult == ProcResult::NotFound) {
		for (int attempt = 0; attempt < 100; ++attempt) {
			std::lock_guard<std::mutex> lock(g_mutex);
			auto it = g_entries.find(pid);
			if (it != g_entries.end() && it->second.exited) {
				return ImmediateExitJson(pid, it->second.startTime,
					it->second.hasStartTime, it->second.hasExitCode,
					it->second.exitCode);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return ImmediateExitJson(pid, startTime, true, false, 0);
	}

	int64_t processGroupId = 0;
	const pid_t groupId = getpgid(pid);
	if (groupId == pid) processGroupId = static_cast<int64_t>(groupId);

	return StartJsonSuccess(pid, startTime, startTimeUtc, processGroupId);
}

std::string ProcessUtils::GetProcessState(int64_t pid, const std::wstring& startTime)
{
	CallGuard callGuard;

	std::string requested;
	if (!ValidPid(pid) || !ParseStartTime(startTime, requested)) {
		return StateJson("Unknown", false, false, 0, kErrorArgument,
			"Invalid PID or StartTime");
	}

	{
		std::lock_guard<std::mutex> lock(g_mutex);
		auto it = g_entries.find(static_cast<pid_t>(pid));
		if (it != g_entries.end()) {
			Entry& entry = it->second;
			if (!entry.hasStartTime || entry.startTime != requested) {
				return StateJson("StartTimeMismatch", false, false, 0,
					kErrorStartTimeMismatch, "StartTime mismatch");
			}
			if (entry.exited) {
				return StateJson("Exited", true, entry.hasExitCode,
					entry.exitCode, kErrorOk, "");
			}
		}
	}

	ProcStat stat;
	const ProcResult result = ReadProcStat(static_cast<pid_t>(pid), stat);
	if (result == ProcResult::NotFound) {
		std::lock_guard<std::mutex> lock(g_mutex);
		auto it = g_entries.find(static_cast<pid_t>(pid));
		if (it != g_entries.end()) {
			if (!it->second.hasStartTime || it->second.startTime != requested) {
				return StateJson("StartTimeMismatch", false, false, 0,
					kErrorStartTimeMismatch, "StartTime mismatch");
			}
			return StateJson("Exited", true, it->second.hasExitCode,
				it->second.exitCode, kErrorOk, "");
		}
		return StateJson("NotFound", false, false, 0, kErrorOk, "");
	}
	if (result == ProcResult::AccessDenied) {
		return StateJson("AccessDenied", false, false, 0,
			kErrorAccessDenied, "Cannot read /proc stat");
	}
	if (result != ProcResult::Ok) {
		return StateJson("Unknown", false, false, 0, kErrorStartTime,
			"Cannot read /proc stat");
	}

	const std::string current = std::to_string(stat.startTime);
	if (current != requested) {
		return StateJson("StartTimeMismatch", false, false, 0,
			kErrorStartTimeMismatch, "StartTime mismatch");
	}
	if (stat.state == 'Z') {
		return StateJson("Zombie", true, false, 0, kErrorOk, "");
	}
	if (stat.state == 'X' || stat.state == 'x') {
		return StateJson("Exited", true, false, 0, kErrorOk, "");
	}
	return StateJson("Running", true, false, 0, kErrorOk, "");
}

std::string ProcessUtils::StopProcess(int64_t pid, const std::wstring& startTime,
	bool force)
{
	CallGuard callGuard;

	std::string requested;
	if (!ValidPid(pid) || !ParseStartTime(startTime, requested)) {
		return StopJsonError(kErrorArgument, "Invalid PID or StartTime", "Unknown");
	}

	bool registered = false;
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		auto it = g_entries.find(static_cast<pid_t>(pid));
		if (it != g_entries.end()) {
			Entry& entry = it->second;
			if (!entry.hasStartTime || entry.startTime != requested) {
				return StopJsonError(kErrorStartTimeMismatch,
					"StartTime mismatch", "StartTimeMismatch");
			}
			registered = true;
			if (entry.exited) {
				return StopJsonSuccess(true, entry.hasExitCode, entry.exitCode, "Exited");
			}
		}
	}

	ProcStat stat;
	const ProcResult result = ReadProcStat(static_cast<pid_t>(pid), stat);
	if (result == ProcResult::NotFound) {
		return StopJsonSuccess(true, false, 0, "NotFound");
	}
	if (result == ProcResult::AccessDenied) {
		return StopJsonError(kErrorAccessDenied,
			"Cannot read /proc stat", "AccessDenied");
	}
	if (result != ProcResult::Ok) {
		return StopJsonError(kErrorStartTime,
			"Cannot read /proc stat", "Unknown");
	}

	const std::string current = std::to_string(stat.startTime);
	if (current != requested) {
		return StopJsonError(kErrorStartTimeMismatch,
			"StartTime mismatch", "StartTimeMismatch");
	}
	if (stat.state == 'Z' || stat.state == 'X' || stat.state == 'x') {
		return StopJsonSuccess(true, false, 0, "Exited");
	}

	const pid_t processId = static_cast<pid_t>(pid);
	const int pidfd = OpenPidFd(processId);

	// Повторная проверка непосредственно перед отправкой сигнала. pidfd,
	// если ядро его поддерживает, удерживает именно этот процесс и не даёт
	// сигналу попасть в процесс с переиспользованным PID.
	ProcStat verified;
	const ProcResult verifyResult = ReadProcStat(processId, verified);
	if (verifyResult == ProcResult::NotFound) {
		if (pidfd >= 0) close(pidfd);
		return StopJsonSuccess(true, false, 0, "Exited");
	}
	if (verifyResult == ProcResult::AccessDenied) {
		if (pidfd >= 0) close(pidfd);
		return StopJsonError(kErrorAccessDenied,
			"Cannot read /proc stat", "AccessDenied");
	}
	if (verifyResult != ProcResult::Ok) {
		if (pidfd >= 0) close(pidfd);
		return StopJsonError(kErrorStartTime,
			"Cannot read /proc stat", "Unknown");
	}
	if (std::to_string(verified.startTime) != requested) {
		if (pidfd >= 0) close(pidfd);
		return StopJsonError(kErrorStartTimeMismatch,
			"StartTime mismatch", "StartTimeMismatch");
	}
	if (verified.state == 'Z' || verified.state == 'X' || verified.state == 'x') {
		if (pidfd >= 0) close(pidfd);
		return StopJsonSuccess(true, false, 0, "Exited");
	}

	const int signal = force ? SIGKILL : SIGTERM;
	const pid_t groupId = getpgid(processId);
	int signalResult = 0;
	if (registered && groupId == processId) {
		signalResult = kill(-processId, signal);
		if (signalResult != 0 && errno == ESRCH && pidfd >= 0) {
			signalResult = SendPidFdSignal(pidfd, signal);
		}
	}
	else if (pidfd >= 0) {
		signalResult = SendPidFdSignal(pidfd, signal);
	}
	else {
		signalResult = kill(processId, signal);
	}
	if (pidfd >= 0) close(pidfd);

	if (signalResult != 0) {
		if (errno == ESRCH) return StopJsonSuccess(true, false, 0, "Exited");
		if (errno == EPERM) {
			return StopJsonError(kErrorAccessDenied,
				"kill: access denied", "AccessDenied");
		}
		return StopJsonError(kErrorInternal,
			"kill failed: " + ErrnoText(errno), "Running");
	}
	return StopJsonSuccess(false, false, 0, "Running");
}

bool ProcessUtils::GetStartTime(int64_t pid, std::string& startTime,
	std::string& startTimeUtc)
{
	CallGuard callGuard;

	startTime.clear();
	startTimeUtc.clear();
	if (!ValidPid(pid)) return false;
	ProcStat stat;
	if (ReadProcStat(static_cast<pid_t>(pid), stat) != ProcResult::Ok) return false;
	startTime = std::to_string(stat.startTime);
	startTimeUtc = FormatStartTimeUtc(stat.startTime);
	return true;
}

void ProcessUtils::AddRef()
{
	g_refs.fetch_add(1);
}

void ProcessUtils::Release()
{
	const int references = g_refs.fetch_sub(1) - 1;
	if (references <= 0) Shutdown();
}

void ProcessUtils::Shutdown()
{
	WaitActiveCallsFinished();

	bool joinReaper = false;
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		if (g_reaperStarted) {
			g_reaperStarted = false;
			g_stop.store(true);
			joinReaper = true;
		}
	}
	if (joinReaper) pthread_join(g_reaperThread, nullptr);

	{
		std::lock_guard<std::mutex> lock(g_mutex);
		for (auto& item : g_entries) {
			if (item.second.exited) continue;
			int status = 0;
			waitpid(item.first, &status, WNOHANG);
		}
		g_entries.clear();
		g_stop.store(false);
	}

	{
		std::lock_guard<std::mutex> lock(g_lifecycleMutex);
		g_shuttingDown = false;
	}
}

#endif
