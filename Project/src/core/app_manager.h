#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <iostream>

/**
 * @brief Structure to hold application information from registry
 */
struct AppInfo
{
	std::string name;
	std::string displayName; // Added for compatibility with server_main.cpp
	std::string installPath;
	std::string version;
};

/**
 * @brief Application management class for managing installed and running applications
 * Simplified implementation using system commands like draft_app_control.cpp
 */
class AppManager
{
public:
	/**
	 * @brief List all installed applications from Windows Registry
	 * Prints application names to console and returns list
	 * @return Vector of installed applications
	 */
	std::vector<AppInfo> listInstalledApps();

	/**
	 * @brief List all installed applications as CSV string with columns: No.,Name,Version,Location
	 * @return CSV formatted string of installed applications
	 */
	std::string listInstalledAppsAsCSV();

	/**
	 * @brief Start an application using system command (like draft implementation)
	 * @param appPath Full path to the executable file
	 * @param arguments Command line arguments (optional)
	 * @return true if application started successfully, false otherwise
	 */
	bool startApp(const std::string &appPath, const std::string &arguments = "");

	/**
	 * @brief Stop a running application by process name using taskkill
	 * @param processName Name of the process to stop (e.g., "notepad.exe")
	 * @return true if process was found and terminated, false otherwise
	 */
	/**
	 * @brief Find a running process by name and return its Process ID
	 * @param processName Name of the process to find (e.g., "notepad.exe")
	 * @return Process ID if found, 0 if not found
	 */
	DWORD findProcessByName(const std::string &processName);

	bool stopApp(const std::string &processName);
};