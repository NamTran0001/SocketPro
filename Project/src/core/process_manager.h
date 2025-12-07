#pragma once

#include <string>
#include <vector>
#include <windows.h>

/**
 * ProcessManager class - Centralized process management for server operations
 *
 * Handles PROCESS_LIST, PROCESS_START, PROCESS_STOP commands as specified in Architecture.md
 * Integrates with server_main.cpp command processing system
 */
class ProcessManager
{
public:
	ProcessManager();
	~ProcessManager();

	// Returns CSV formatted process list with columns: Name,PID,Session Name,Session#,Mem Usage
	std::string getProcessList();

	// Architecture.md: PROCESS_START command implementation
	bool startProcess(const std::string &appPath);

	// Architecture.md: PROCESS_STOP command implementation
	// Uses system command 'taskkill' instead of Windows API
	bool stopProcessByPID(DWORD pid);

	// Uses system command 'taskkill' instead of Windows API
	bool stopProcessByName(const std::string &processName);

private:
	// Format: "processname.exe x[count]" for grouped display
	std::string getFormattedProcessList();

	// Uses tasklist command with CSV output format
	std::string getDetailedProcessList();

	bool validatePath(const std::string &path);
};