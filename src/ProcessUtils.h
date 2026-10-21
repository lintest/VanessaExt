#pragma once

#include <cstdint>
#include <string>

// Кроссплатформенный запуск и сопровождение процессов по паре PID + StartTime.
// StartProcess/StopProcess/GetProcessState возвращают JSON в UTF-8; конвертацию
// в std::wstring выполняет ProcessManager на границе компоненты.
// GetStartTime возвращает каноническую строку StartTime и UTC-представление.
class ProcessUtils
{
public:
	static std::string StartProcess(const std::wstring& command,
	                                const std::wstring& windowMode);
	static std::string StopProcess(int64_t pid,
	                               const std::wstring& startTime,
	                               bool force);
	static std::string GetProcessState(int64_t pid,
	                                   const std::wstring& startTime);

	// Канонический StartTime процесса. Для уже завершившегося процесса
	// на Windows значение доступно, пока открыт handle; на Linux — пока
	// существует запись /proc.
	static bool GetStartTime(int64_t pid,
	                         std::string& startTime,
	                         std::string& startTimeUtc);

	// Счётчик экземпляров компоненты. Первый AddRef запускает reaper,
	// последний Release останавливает его.
	static void AddRef();
	static void Release();
	static void Shutdown();
};
