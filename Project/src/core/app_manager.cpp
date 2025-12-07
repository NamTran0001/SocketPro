#include "app_manager.h"
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include <cctype>

// Windows API includes for registry operations
#include <Windows.h>
#include <winreg.h>

/**
 * @brief Execute a shell command and return output as string
 * Uses _popen on Windows for command execution
 * @param cmd Command to execute
 * @return Command output as string
 */
std::string executeCommand(const char *cmd)
{
	char buffer[128];
	std::string result = "";
	FILE *pipe = _popen(cmd, "r");
	if (!pipe)
	{
		return "Error executing command!";
	}
	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		result += buffer;
	}
	_pclose(pipe);
	return result;
}

std::vector<AppInfo> AppManager::listInstalledApps()
{
	std::vector<AppInfo> apps;
	std::cout << "Retrieving list of installed applications...\n";

	// PowerShell command to get installed applications from registry
	// Outputs as CSV format for easier parsing
	const char *command = "powershell \"Get-ItemProperty HKLM:\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*, HKLM:\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\* | Where-Object { $_.DisplayName -ne $null -and $_.InstallLocation -ne $null } | Select-Object DisplayName, InstallLocation, DisplayVersion | ConvertTo-Csv -NoTypeInformation\"";

	std::string commandOutput = executeCommand(command);
	std::stringstream ss(commandOutput);
	std::string line;

	// Skip CSV header line
	std::getline(ss, line);

	// Parse each line of CSV output
	while (std::getline(ss, line))
	{
		std::stringstream lineStream(line);
		std::string cell;
		std::vector<std::string> cells;

		// Parse CSV line
		while (std::getline(lineStream, cell, ','))
		{
			// Remove quotes from beginning and end
			if (!cell.empty() && cell.front() == '"')
			{
				cell.erase(0, 1);
			}
			if (!cell.empty() && cell.back() == '"')
			{
				cell.pop_back();
			}
			cells.push_back(cell);
		}

		if (cells.size() >= 3)
		{
			AppInfo app;
			app.name = cells[0];
			app.displayName = cells[0]; // Copy name to displayName for compatibility
			app.installPath = cells[1];
			app.version = cells[2];
			apps.push_back(app);
		}
	}

	return apps;
}

std::string AppManager::listInstalledAppsAsCSV()
{
	std::vector<AppInfo> apps = listInstalledApps();
	std::stringstream csvStream;

	// CSV header
	csvStream << "No.,Name,Version,Location" << std::endl;

	// CSV data rows
	for (size_t i = 0; i < apps.size(); ++i)
	{
		csvStream << (i + 1) << ",\"" << apps[i].displayName << "\",\""
							<< apps[i].version << "\",\"" << apps[i].installPath << "\"" << std::endl;
	}

	return csvStream.str();
}

bool AppManager::startApp(const std::string &appPath, const std::string &arguments)
{
	// Check if appPath is a full path or just an app name
	bool isFullPath = (appPath.find('\\') != std::string::npos) || (appPath.find('/') != std::string::npos);

	std::cout << "Starting " << appPath;
	if (!arguments.empty())
	{
		std::cout << " with arguments: " << arguments;
	}
	std::cout << "...\n";

	if (isFullPath)
	{
		// Build start command with optional arguments for full path
		std::string command = "start \"\" \"" + appPath + "\"";
		if (!arguments.empty())
		{
			command += " " + arguments;
		}
		int result = system(command.c_str());
		if (result == 0)
		{
			std::cout << "Application started successfully!\n";
			return true;
		}
		else
		{
			std::cout << "Failed to start application. Please check the path.\n";
			return false;
		}
	}
	else
	{
		// Use ShellExecute to launch app by name (from PATH)
		HINSTANCE hInst = ShellExecuteA(
				NULL,
				"open",
				appPath.c_str(),
				arguments.empty() ? NULL : arguments.c_str(),
				NULL,
				SW_SHOWNORMAL);

		if ((INT_PTR)hInst > 32)
		{
			std::cout << "Application started successfully!\n";
			return true;
		}
		else
		{
			std::cout << "Failed to start application. Please check the app name.\n";
			return false;
		}
	}
}

DWORD AppManager::findProcessByName(const std::string &processName)
{
	// Use PowerShell to find process by name and return PID
	std::string command = "powershell \"Get-Process -Name '" + processName + "' -ErrorAction SilentlyContinue | Select-Object -First 1 | ForEach-Object { $_.Id }\"";

	// Remove .exe extension if present for Get-Process command
	std::string cleanProcessName = processName;
	size_t exePos = cleanProcessName.find(".exe");
	if (exePos != std::string::npos)
	{
		cleanProcessName = cleanProcessName.substr(0, exePos);
	}

	command = "powershell \"Get-Process -Name '" + cleanProcessName + "' -ErrorAction SilentlyContinue | Select-Object -First 1 | ForEach-Object { $_.Id }\"";

	std::string result = executeCommand(command.c_str());

	// Parse the result to get PID
	if (!result.empty())
	{
		try
		{
			// Remove any whitespace/newlines
			result.erase(result.find_last_not_of(" \n\r\t") + 1);
			if (!result.empty() && std::all_of(result.begin(), result.end(), ::isdigit))
			{
				return static_cast<DWORD>(std::stoul(result));
			}
		}
		catch (const std::exception &)
		{
			// If conversion fails, return 0
		}
	}

	return 0; // Process not found
}

bool AppManager::stopApp(const std::string &processName)
{
	if (processName.empty())
	{
		std::cout << "Error: Process name cannot be empty\n";
		return false;
	}

	// Create a copy to work with
	std::string actualProcessName = processName;

	// Ensure the process name has .exe extension if not already present
	if (actualProcessName.length() < 4 ||
			actualProcessName.substr(actualProcessName.length() - 4) != ".exe")
	{
		actualProcessName += ".exe";
	}

	// Use taskkill command to terminate process by name
	std::string command = "taskkill /IM \"" + actualProcessName + "\" /F";

	std::cout << "Stopping process '" << actualProcessName << "'...\n";

	int result = system(command.c_str());

	if (result == 0)
	{
		std::cout << "Process '" << actualProcessName << "' stopped successfully!\n";
		return true;
	}
	else
	{
		std::cout << "Failed to stop process '" << actualProcessName << "'. Process name might be incorrect or process doesn't exist.\n";

		// If the original name already had .exe, try without it as backup
		if (processName.length() >= 4 &&
				processName.substr(processName.length() - 4) == ".exe")
		{
			std::string nameWithoutExt = processName.substr(0, processName.length() - 4);
			std::string backupCommand = "taskkill /IM \"" + nameWithoutExt + "\" /F";
			std::cout << "Trying alternative name '" << nameWithoutExt << "'...\n";

			int backupResult = system(backupCommand.c_str());
			if (backupResult == 0)
			{
				std::cout << "Process '" << nameWithoutExt << "' stopped successfully!\n";
				return true;
			}
		}

		return false;
	}
}