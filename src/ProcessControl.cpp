#include "ProcessControl.h"

#ifndef _WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include <chrono>
#include <thread>
#define PIPE_READ 0
#define PIPE_WRITE 1
#endif

std::vector<std::u16string> ProcessControl::names = {
	AddComponent(u"ProcessControl", []() { return new ProcessControl; }),
};

ProcessControl::ProcessControl()
{
	AddProperty(u"ProcessId", u"ИдентификаторПроцесса",
		[&](VH var) { var = this->ProcessId(); }
	);
	AddProperty(u"ExitCode", u"КодВозврата",
		[&](VH var) { var = this->ExitCode(); }
	);
	AddProperty(u"IsActive", u"Активный",
		[&](VH var) { var = this->IsActive(); }
	);

	AddFunction(u"Create", u"Создать",
		[&](VH command, VH show) { this->result = this->Create(command, show); },
		{ {1, false} }
	);

	AddFunction(u"Wait", u"Ждать", 
		[&](VH msec) { this->result = this->Wait(msec); }
	);

	AddFunction(u"InputData", u"ВвестиДанные",
		[&](VH text) { this->result = this->Input(text); },
		{ {1, false} }
	);

	AddFunction(u"Terminate", u"Прервать"
		, [&]() { this->result = this->Terminate(); }
	);

	AddProcedure(u"Sleep", u"Пауза", [&](VH msec) { this->Sleep(msec); });

#ifdef _WINDOWS
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	CreatePipe(&hInPipeR, &hInPipeW, &saAttr, 0);
	CreatePipe(&hOutPipeR, &hOutPipeW, &saAttr, 0);
	SetHandleInformation(hInPipeW, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hOutPipeR, HANDLE_FLAG_INHERIT, 0);
#else
	(void)!pipe(m_pipe);
#endif //_WINDOWS
}

#ifdef _WINDOWS

ProcessControl::~ProcessControl()
{
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

bool ProcessControl::Create(std::wstring command, bool show)
{
	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdInput = hInPipeR;
	si.dwFlags = STARTF_USESTDHANDLES;
	if (!show) {
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}
	return ::CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
}

bool ProcessControl::Terminate()
{
	return ::TerminateProcess(pi.hProcess, 0);
}

int64_t ProcessControl::Wait(int64_t msec)
{
	if (msec == 0) msec = INFINITE;
	switch (::WaitForSingleObject(pi.hProcess, (DWORD)msec)) {
	case 0: return 0;
	case WAIT_TIMEOUT: return 1;
	default: return -1;
	}
}

bool ProcessControl::Input(const std::string &text)
{
	DWORD dwWritten;
	return ::WriteFile(hInPipeW, text.c_str(), (DWORD)text.size(), &dwWritten, NULL);
}

void ProcessControl::Sleep(int64_t msec)
{
	::Sleep((DWORD)msec);
}

int64_t ProcessControl::ProcessId()
{
	return (int32_t)pi.dwProcessId;
}

int64_t ProcessControl::ExitCode()
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

void ProcessControl::ClosePipe()
{
	if (m_pipe[PIPE_READ] >= 0) {
		close(m_pipe[PIPE_READ]);
		m_pipe[PIPE_READ] = -1;
	}
	if (m_pipe[PIPE_WRITE] >= 0) {
		close(m_pipe[PIPE_WRITE]);
		m_pipe[PIPE_WRITE] = -1;
	}
}

ProcessControl::~ProcessControl()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_pid > 0 && !m_exited) {
			if (m_processGroupId == m_pid) kill(-m_pid, SIGKILL);
			else kill(m_pid, SIGKILL);
			for (int attempt = 0; attempt < 100; ++attempt) {
				if (PollStateLocked() == 1) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
	}
	ClosePipe();
}

int ProcessControl::PollStateLocked()
{
	if (m_pid <= 0) return -1;
	if (m_exited) return 1;

	int status = 0;
	pid_t result = 0;
	do {
		result = waitpid(m_pid, &status, WNOHANG);
	} while (result == -1 && errno == EINTR);

	if (result == 0) return 0;
	if (result == m_pid) {
		m_exited = true;
		m_hasExitCode = false;
		if (WIFEXITED(status)) {
			m_hasExitCode = true;
			m_exitCode = WEXITSTATUS(status);
		}
		else if (WIFSIGNALED(status)) {
			m_hasExitCode = true;
			m_exitCode = 128 + WTERMSIG(status);
		}
		return 1;
	}
	if (errno == ECHILD) {
		// Процесс уже собран другим кодом; настоящий код возврата неизвестен.
		m_exited = true;
		m_hasExitCode = false;
		m_exitCode = -1;
		return 1;
	}
	return -1;
}

int ProcessControl::PollState()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return PollStateLocked();
}

bool ProcessControl::Create(std::wstring command, bool show)
{
	(void)show;
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_pid > 0 && !m_exited) {
		const int state = PollStateLocked();
		// При ошибке waitpid нельзя терять старый дочерний процесс.
		if (state != 1) return false;
	}

	ClosePipe();
	m_pid = -1;
	m_processGroupId = -1;
	m_exited = false;
	m_hasExitCode = false;
	m_exitCode = 0;

	const std::string cmd = WC2MB(command);
	if (cmd.empty()) return false;
	if (pipe(m_pipe) != 0) {
		m_pipe[PIPE_READ] = -1;
		m_pipe[PIPE_WRITE] = -1;
		return false;
	}

	// Оболочка нужна для пайпов и перенаправлений. Собственная группа
	// процессов позволяет завершить и её дочерние процессы.
	const std::string shellCommand = cmd;
	const pid_t child = fork();
	if (child == 0) {
		setpgid(0, 0);
		if (dup2(m_pipe[PIPE_READ], STDIN_FILENO) == -1) _exit(127);
		close(m_pipe[PIPE_READ]);
		close(m_pipe[PIPE_WRITE]);
		execl("/bin/sh", "sh", "-c", shellCommand.c_str(), nullptr);
		_exit(127);
	}
	if (child < 0) {
		ClosePipe();
		return false;
	}

	setpgid(child, child);
	m_pid = child;
	m_processGroupId = (getpgid(child) == child) ? child : -1;
	close(m_pipe[PIPE_READ]);
	m_pipe[PIPE_READ] = -1;
	return true;
}

bool ProcessControl::Terminate()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_pid <= 0) return false;

		const int state = PollStateLocked();
		if (state == 1) return true;
		if (state < 0) return false;

		int result = 0;
		pid_t groupId = m_processGroupId;
		if (groupId != m_pid) {
			// Дочерний setpgid мог выполниться уже после возврата из fork.
			const pid_t currentGroup = getpgid(m_pid);
			if (currentGroup == m_pid) groupId = m_pid;
		}
		if (groupId == m_pid) {
			result = kill(-m_pid, SIGKILL);
		}
		else {
			result = kill(m_pid, SIGKILL);
		}
		if (result != 0) {
			if (errno == ESRCH) {
				return PollStateLocked() == 1;
			}
			return false;
		}
	}

	// kill возвращается до фактической смерти процесса. Ждём reap,
	// чтобы следующий Create не увидел ещё активный PID.
	for (int attempt = 0; attempt < 100; ++attempt) {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (PollStateLocked() == 1) return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return false;
}

bool ProcessControl::Input(const std::string& text)
{
	int fd = -1;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_pipe[PIPE_WRITE] < 0) return false;
		fd = dup(m_pipe[PIPE_WRITE]);
	}
	if (fd < 0) return false;

	sigset_t pipeSet;
	sigset_t oldSet;
	sigemptyset(&pipeSet);
	sigaddset(&pipeSet, SIGPIPE);
	const bool maskChanged = pthread_sigmask(SIG_BLOCK, &pipeSet, &oldSet) == 0;

	size_t written = 0;
	while (written < text.size()) {
		const ssize_t result = write(fd,
			text.data() + written, text.size() - written);
		if (result < 0) {
			if (errno == EINTR) continue;
			if (errno == EPIPE) {
				struct timespec timeout = { 0, 0 };
				sigtimedwait(&pipeSet, nullptr, &timeout);
			}
			break;
		}
		if (result == 0) break;
		written += static_cast<size_t>(result);
	}

	if (maskChanged) pthread_sigmask(SIG_SETMASK, &oldSet, nullptr);
	close(fd);
	return written == text.size();
}

void ProcessControl::Sleep(int64_t msec)
{
	if (msec <= 0) return;
	std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

int64_t ProcessControl::Wait(int64_t msec)
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_pid <= 0) return -1;
	}
	if (msec <= 0) {
		while (true) {
			const int state = PollState();
			if (state == 1) return 0;
			if (state < 0) return -1;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	const auto deadline = std::chrono::steady_clock::now() +
		std::chrono::milliseconds(msec);
	while (std::chrono::steady_clock::now() < deadline) {
		const int state = PollState();
		if (state == 1) return 0;
		if (state < 0) return -1;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	const int state = PollState();
	if (state == 1) return 0;
	if (state < 0) return -1;
	return 1;
}

int64_t ProcessControl::ProcessId()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return (int64_t)m_pid;
}

int64_t ProcessControl::ExitCode()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	const int state = PollStateLocked();
	if (state < 0) return -1;
	if (state == 0) return 259;
	if (!m_hasExitCode) return -1;
	return m_exitCode;
}

bool ProcessControl::IsActive()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return PollStateLocked() == 0;
}

#endif //_WINDOWS
