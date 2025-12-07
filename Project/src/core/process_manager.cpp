#include "process_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <windows.h> // Still needed for DWORD, MAX_PATH, and startProcess

ProcessManager::ProcessManager()
{
	// Initialize process manager
}

ProcessManager::~ProcessManager()
{
	// Cleanup if needed
}

std::string ProcessManager::getProcessList()
{
	// Use tasklist with CSV format - provides all required columns:
	// "Image Name","PID","Session Name","Session#","Mem Usage"
	FILE *pipe = _popen("tasklist /FO CSV", "r");
	if (!pipe)
	{
		std::cerr << "Cannot run tasklist command." << std::endl;
		return "Name,PID,Session Name,Session#,Mem Usage\nERROR,0,N/A,0,0 K";
	}

	std::ostringstream result;
	char buffer[1024];
	bool headerProcessed = false;

	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		std::string line(buffer);

		// Process header line to ensure correct column names
		if (!headerProcessed && line.find("Image Name") != std::string::npos)
		{
			// Replace "Image Name" with "Name" to match required format
			size_t pos = line.find("Image Name");
			if (pos != std::string::npos)
			{
				line.replace(pos, 12, "Name");
			}
			headerProcessed = true;
		}

		result << line;
	}

	int exitCode = _pclose(pipe);
	if (exitCode != 0)
	{
		std::cerr << "Warning: tasklist command completed with code " << exitCode << std::endl;
	}

	return result.str();
}

bool ProcessManager::startProcess(const std::string &appPath)
{
	if (appPath.empty() || !validatePath(appPath))
	{
		std::cerr << "Invalid or inaccessible path: " << appPath << std::endl;
		return false;
	}

	if (appPath.length() >= MAX_PATH)
	{
		std::cerr << "Path too long: " << appPath << std::endl;
		return false;
	}

	STARTUPINFOA si = {0};
	si.cb = sizeof(STARTUPINFOA);
	PROCESS_INFORMATION pi = {0};

	// Create a modifiable copy of the path
	char executablePath[MAX_PATH];
	strncpy(executablePath, appPath.c_str(), MAX_PATH - 1);
	executablePath[MAX_PATH - 1] = '\0';

	if (CreateProcessA(
					nullptr,				// lpApplicationName
					executablePath, // lpCommandLine
					nullptr,				// lpProcessAttributes
					nullptr,				// lpThreadAttributes
					FALSE,					// bInheritHandles
					0,							// dwCreationFlags
					nullptr,				// lpEnvironment
					nullptr,				// lpCurrentDirectory
					&si,						// lpStartupInfo
					&pi))						// lpProcessInformation
	{
		std::cout << "Process started successfully: " << appPath
							<< " (PID: " << pi.dwProcessId << ")" << std::endl;

		// Close handles to avoid resource leaks
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	DWORD error = GetLastError();
	std::cerr << "Failed to start process: " << appPath
						<< " | Error Code: " << error << std::endl;
	return false;
}

bool ProcessManager::stopProcessByPID(DWORD pid)
{
	if (pid == 0)
	{
		std::cerr << "Invalid PID: 0" << std::endl;
		return false;
	}

	// Priority: Use system command (taskkill) instead of Windows API
	std::string cmd = "taskkill /F /PID " + std::to_string(pid) + " 2>&1";

	FILE *pipe = _popen(cmd.c_str(), "r");
	if (!pipe)
	{
		std::cerr << "Failed to execute taskkill command for PID " << pid << std::endl;
		return false;
	}

	char buffer[512];
	std::string output;
	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		output += buffer;
	}

	int result = _pclose(pipe);

	if (result == 0)
	{
		std::cout << "Successfully terminated process PID: " << pid << std::endl;
		return true;
	}
	else
	{
		// Check if error is "process not found" vs actual error
		if (output.find("not found") != std::string::npos ||
				output.find("không tìm thấy") != std::string::npos)
		{
			std::cerr << "Process not found with PID: " << pid << std::endl;
		}
		else
		{
			std::cerr << "Failed to terminate process PID " << pid
								<< " | Output: " << output << std::endl;
		}
		return false;
	}
}

bool ProcessManager::stopProcessByName(const std::string &processName)
{
	if (processName.empty())
	{
		std::cerr << "Empty process name provided" << std::endl;
		return false;
	}

	// Priority: Use system command (taskkill) instead of Windows API
	std::string cmd = "taskkill /F /IM \"" + processName + "\"";

	// Redirect stderr to capture error messages
	cmd += " 2>&1";

	FILE *pipe = _popen(cmd.c_str(), "r");
	if (!pipe)
	{
		std::cerr << "Failed to execute taskkill command" << std::endl;
		return false;
	}

	char buffer[512];
	std::string output;
	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		output += buffer;
	}

	int result = _pclose(pipe);

	if (result == 0)
	{
		std::cout << "Successfully terminated process: " << processName << std::endl;
		return true;
	}
	else
	{
		// Check if error is "process not found" vs actual error
		if (output.find("not found") != std::string::npos ||
				output.find("không tìm thấy") != std::string::npos)
		{
			std::cerr << "Process not found: " << processName << std::endl;
		}
		else
		{
			std::cerr << "Failed to terminate process " << processName
								<< " | Output: " << output << std::endl;
		}
		return false;
	}
}

std::string ProcessManager::getFormattedProcessList()
{
	FILE *pipe = _popen("tasklist", "r");
	if (!pipe)
	{
		return "Cannot run tasklist command.";
	}

	std::ostringstream result;
	char buffer[512];
	std::string currentProcess;
	int count = 0;
	bool firstOutput = true;

	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		std::string line(buffer);

		// Skip header lines
		if (line.find("Image Name") != std::string::npos ||
				line.find("=") != std::string::npos)
			continue;

		// Extract process name (first token)
		std::istringstream iss(line);
		std::string processName;
		iss >> processName;

		if (processName.empty())
			continue;

		if (currentProcess.empty())
		{
			// First process
			currentProcess = processName;
			count = 1;
		}
		else if (processName == currentProcess)
		{
			// Same process as previous - increment count
			count++;
		}
		else
		{
			// New process - output previous process group
			if (!firstOutput)
				result << "\n";
			result << currentProcess << " x" << count;
			firstOutput = false;

			// Reset for new process
			currentProcess = processName;
			count = 1;
		}
	}
	_pclose(pipe);

	// Output last process group
	if (!currentProcess.empty())
	{
		if (!firstOutput)
			result << "\\n";
		result << currentProcess << " x" << count;
	}

	std::string output = result.str();
	if (output.empty())
		output = "No processes running.";

	return output;
}

std::string ProcessManager::getDetailedProcessList()
{
	FILE *pipe = _popen("tasklist /FO CSV", "r");
	if (!pipe)
	{
		return "Cannot run tasklist command.";
	}

	std::ostringstream result;
	char buffer[512];

	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		result << buffer;
	}

	_pclose(pipe);
	return result.str();
}

bool ProcessManager::validatePath(const std::string &path)
{
	if (path.empty())
		return false;

	// Check if path is absolute (contains : for drive letter or starts with \\\\)
	if (path.length() < 3 ||
			(path[1] != ':' && path.substr(0, 2) != "\\\\\\\\"))
	{
		std::cerr << "Path must be absolute: " << path << std::endl;
		return false;
	}

	// Check if file exists and is accessible
	std::filesystem::path filePath(path);
	if (!std::filesystem::exists(filePath))
	{
		std::cerr << "File does not exist: " << path << std::endl;
		return false;
	}

	if (!std::filesystem::is_regular_file(filePath))
	{
		std::cerr << "Path is not a regular file: " << path << std::endl;
		return false;
	}

	return true;
}