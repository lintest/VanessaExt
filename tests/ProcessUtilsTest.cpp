#include "stdafx.h"
#include "ProcessUtils.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#ifndef _WINDOWS
#include <sys/stat.h>
#include <unistd.h>
#endif

static JSON Parse(const std::string& text)
{
	try {
		return JSON::parse(text);
	}
	catch (const std::exception& error) {
		std::cerr << "Cannot parse JSON: " << error.what() << "\n" << text << "\n";
		std::exit(1);
	}
}

static void Require(bool condition, const char* message)
{
	if (condition) return;
	std::cerr << "FAILED: " << message << "\n";
	std::exit(1);
}

static std::wstring Widen(const std::string& text)
{
	return std::wstring(text.begin(), text.end());
}

static JSON WaitFinished(int64_t pid, const std::wstring& startTime)
{
	const auto deadline = std::chrono::steady_clock::now() +
		std::chrono::seconds(5);
	while (std::chrono::steady_clock::now() < deadline) {
		JSON state = Parse(ProcessUtils::GetProcessState(pid, startTime));
		const std::string value = state["State"].get<std::string>();
		if (value == "Exited" || value == "NotFound" || value == "Zombie") {
			return state;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	std::cerr << "Process did not finish: " << pid << "\n";
	std::exit(1);
}

int main()
{
	ProcessUtils::AddRef();

	JSON invalid = Parse(ProcessUtils::StartProcess(L"", L"Normal"));
	Require(!invalid["ok"].get<bool>(), "empty command must fail");
	Require(invalid["errorCode"].get<int>() == 1, "empty command errorCode");

	JSON badMode = Parse(ProcessUtils::StartProcess(L"/bin/sleep 1", L"Unknown"));
	Require(!badMode["ok"].get<bool>(), "unknown window mode must fail");

#ifdef _WINDOWS
	const std::wstring command = L"ping.exe -n 30 127.0.0.1";
	const std::wstring immediate = L"cmd /c exit 7";
#else
	const std::wstring command = L"/bin/sleep 30";
	const std::wstring immediate = L"/bin/false";
#endif

	JSON started = Parse(ProcessUtils::StartProcess(command, L"Hidden"));
	Require(started["ok"].get<bool>(), "process must start");
	const int64_t pid = started["ProcessId"].get<int64_t>();
	const std::string startTime = started["StartTime"].get<std::string>();
	Require(pid > 0, "PID must be positive");
	Require(!startTime.empty(), "StartTime must not be empty");
	Require(std::stoull(startTime) > 0, "StartTime must be the kernel starttime");
	Require(!started["StartTimeUTC"].get<std::string>().empty(),
		"StartTimeUTC must not be empty");
	Require(started["State"].get<std::string>() == "Running", "start state");

	JSON running = Parse(ProcessUtils::GetProcessState(pid, Widen(startTime)));
	Require(running["State"].get<std::string>() == "Running", "process must be running");
	Require(running["StartTimeMatches"].get<bool>(), "StartTime must match");

	JSON mismatch = Parse(ProcessUtils::GetProcessState(pid, L"1"));
	Require(mismatch["State"].get<std::string>() == "StartTimeMismatch",
		"wrong StartTime must not match");
	Require(mismatch["errorCode"].get<int>() == 4,
		"wrong StartTime state errorCode");

	JSON wrongStop = Parse(ProcessUtils::StopProcess(pid, L"1", true));
	Require(!wrongStop["ok"].get<bool>(), "wrong StartTime must not stop");
	Require(wrongStop["errorCode"].get<int>() == 4, "wrong StartTime errorCode");

	JSON stopped = Parse(ProcessUtils::StopProcess(pid, Widen(startTime), true));
	Require(stopped["ok"].get<bool>(), "force stop must be accepted");
	WaitFinished(pid, Widen(startTime));

	JSON stoppedAgain = Parse(ProcessUtils::StopProcess(pid, Widen(startTime), true));
	Require(stoppedAgain["ok"].get<bool>(), "second stop must be idempotent");
	Require(stoppedAgain["AlreadyExited"].get<bool>(), "second stop must report exit");

	JSON immediateResult = Parse(ProcessUtils::StartProcess(immediate, L"Hidden"));
	if (immediateResult["ok"].get<bool>()) {
		const int64_t immediatePid = immediateResult["ProcessId"].get<int64_t>();
		const std::wstring immediateStart = Widen(
			immediateResult["StartTime"].get<std::string>());
		JSON finished = WaitFinished(immediatePid, immediateStart);
		Require(finished["State"].get<std::string>() == "Exited",
			"immediate process must finish");
	}
	else {
		const int immediateCode = immediateResult["errorCode"].get<int>();
		Require(immediateCode == 8 || immediateCode == 9,
			"immediate exit must return 8 or 9");
		Require(immediateResult["ProcessId"].get<int64_t>() > 0,
			"immediate exit must return ProcessId");
	}

	JSON invalidPid = Parse(ProcessUtils::GetProcessState(0, L"1"));
	Require(invalidPid["State"].get<std::string>() == "Unknown", "invalid PID state");

	JSON zeroStart = Parse(ProcessUtils::GetProcessState(pid, L"0"));
	Require(zeroStart["State"].get<std::string>() == "Unknown",
		"zero StartTime must be rejected");

	JSON missing = Parse(ProcessUtils::StartProcess(L"/definitely/not/here", L"Hidden"));
	Require(!missing["ok"].get<bool>(), "missing executable must fail");
	Require(missing["errorCode"].get<int>() == 2,
		"missing executable must return start error");

#ifndef _WINDOWS
	JSON special = Parse(ProcessUtils::StartProcess(
		L"/bin/sh -c \"sleep 30\" \"a$b`c\\d\"", L"Hidden"));
	Require(special["ok"].get<bool>(),
		"quoted shell special characters must be passed literally");
	const int64_t specialPid = special["ProcessId"].get<int64_t>();
	const std::wstring specialStart = Widen(special["StartTime"].get<std::string>());
	JSON specialStopped = Parse(ProcessUtils::StopProcess(specialPid, specialStart, true));
	Require(specialStopped["ok"].get<bool>(), "special-command stop");
	WaitFinished(specialPid, specialStart);

	const std::string script = "/tmp/vext_process_utils_test_" +
		std::to_string(static_cast<long long>(getpid())) + ".sh";
	{
		std::ofstream file(script.c_str(), std::ios::binary | std::ios::trunc);
		file << "#!/bin/sh\nsleep 30\n";
	}
	chmod(script.c_str(), 0700);
	JSON scriptResult = Parse(ProcessUtils::StartProcess(Widen(script), L"Hidden"));
	Require(scriptResult["ok"].get<bool>(), "shebang script must start");
	const int64_t scriptPid = scriptResult["ProcessId"].get<int64_t>();
	const std::wstring scriptStart = Widen(scriptResult["StartTime"].get<std::string>());
	JSON scriptStopped = Parse(ProcessUtils::StopProcess(scriptPid, scriptStart, true));
	Require(scriptStopped["ok"].get<bool>(), "script stop");
	WaitFinished(scriptPid, scriptStart);
	std::remove(script.c_str());
#endif

	ProcessUtils::Release();
	std::cout << "ProcessUtils tests passed\n";
	return 0;
}
