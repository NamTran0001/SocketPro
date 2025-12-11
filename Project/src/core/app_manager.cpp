#include "app_manager.h"
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <Windows.h>
#include <winreg.h>
#include <shellapi.h>

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

std::vector<AppInfo> AppManager::cachedApps;
bool AppManager::appsCacheValid = false;

std::vector<AppInfo> AppManager::listInstalledApps()
{
    // Return cached version if available
    if (appsCacheValid && !cachedApps.empty())
    {
        std::cout << "Using cached app list (" << cachedApps.size() << " apps)\n";
        return cachedApps;
    }
    
    std::cout << "Retrieving list of installed applications (this may take a moment)...\n";

    const char *command = "powershell \"Get-ItemProperty HKLM:\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*, HKLM:\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\* | Where-Object { $_.DisplayName -ne $null } | Select-Object DisplayName, InstallLocation, DisplayVersion, DisplayIcon | ConvertTo-Csv -NoTypeInformation\"";

    std::string commandOutput = executeCommand(command);
    std::stringstream ss(commandOutput);
    std::string line;

    std::getline(ss, line); // Skip header

    cachedApps.clear();
    
    while (std::getline(ss, line))
    {
        std::stringstream lineStream(line);
        std::string cell;
        std::vector<std::string> cells;

        while (std::getline(lineStream, cell, ','))
        {
            if (!cell.empty() && cell.front() == '"') cell.erase(0, 1);
            if (!cell.empty() && cell.back() == '"') cell.pop_back();
            cells.push_back(cell);
        }

        if (cells.size() >= 1 && !cells[0].empty())
        {
            AppInfo app;
            app.name = cells[0];
            app.displayName = cells[0];
            app.installPath = (cells.size() > 1) ? cells[1] : "";
            app.version = (cells.size() > 2) ? cells[2] : "";
            
            // Logic cải tiến: Ưu tiên lấy đường dẫn .exe từ DisplayIcon (cột 4)
            std::string displayIcon = (cells.size() > 3) ? cells[3] : "";
            
            bool installPathIsExe = (app.installPath.find(".exe") != std::string::npos);
            bool iconIsExe = (displayIcon.find(".exe") != std::string::npos);

            if (!installPathIsExe && iconIsExe)
            {
                std::string cleanIcon = displayIcon;
                if (!cleanIcon.empty() && cleanIcon.front() == '"') cleanIcon.erase(0, 1);
                
                size_t exePos = cleanIcon.find(".exe");
                if (exePos != std::string::npos)
                {
                    app.installPath = cleanIcon.substr(0, exePos + 4);
                }
            }
            else if (app.installPath.empty() && iconIsExe)
            {
                size_t exePos = displayIcon.find(".exe");
                if (exePos != std::string::npos)
                {
                    app.installPath = displayIcon.substr(0, exePos + 4);
                }
            }
            
            cachedApps.push_back(app);
        }
    }

    appsCacheValid = true;
    std::cout << "Loaded " << cachedApps.size() << " applications into cache\n";
    return cachedApps;
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

// SỬA LỖI: Cập nhật chữ ký hàm để khớp với header (thêm const ref và arguments)
bool AppManager::startApp(const std::string &appName, const std::string &arguments)
{
    // 1. Đảm bảo cache đã được load
    if (cachedApps.empty()) {
        listInstalledApps();
    }

    std::string executablePath = appName;
    bool foundInCache = false;

    // 2. Tìm kiếm trong danh sách ứng dụng đã cache
    if (!cachedApps.empty()) 
    {
        std::string searchName = appName;
        std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);

        for (const auto& app : cachedApps) 
        {
            std::string cachedName = app.name;
            std::transform(cachedName.begin(), cachedName.end(), cachedName.begin(), ::tolower);

            if (cachedName.find(searchName) != std::string::npos) 
            {
                // SỬA LỖI: Dùng app.installPath thay vì app.installLocation
                if (!app.installPath.empty() && app.installPath.find(".exe") != std::string::npos) {
                    executablePath = app.installPath;
                    foundInCache = true;
                    std::cout << "Found app in cache: " << app.name << " -> " << executablePath << "\n";
                    break; 
                }
            }
        }
    }

    // 3. Sử dụng ShellExecute
    const char* args = arguments.empty() ? NULL : arguments.c_str();
    
    HINSTANCE result = ShellExecuteA(NULL, "open", executablePath.c_str(), args, NULL, SW_SHOWNORMAL);

    if ((intptr_t)result > 32) {
        std::cout << "Successfully started: " << executablePath << "\n";
        return true;
    } 
    
    // 4. Fallback: Thử thêm đuôi .exe
    if (executablePath.find(".exe") == std::string::npos) {
        std::string exePath = executablePath + ".exe";
        result = ShellExecuteA(NULL, "open", exePath.c_str(), args, NULL, SW_SHOWNORMAL);
        
        if ((intptr_t)result > 32) {
             std::cout << "Successfully started (with .exe): " << exePath << "\n";
             return true;
        }
    }

    std::cout << "Failed to start app: " << appName << " (Error code: " << (intptr_t)result << ")\n";
    return false;
}

DWORD AppManager::findProcessByName(const std::string &processName)
{
    // Use PowerShell to find process by name and return PID
    std::string command = "powershell \"Get-Process -Name '" + processName + "' -ErrorAction SilentlyContinue | Select-Object -First 1 | ForEach-Object { $_.Id }\"";

    std::string cleanProcessName = processName;
    size_t exePos = cleanProcessName.find(".exe");
    if (exePos != std::string::npos)
    {
        cleanProcessName = cleanProcessName.substr(0, exePos);
    }

    command = "powershell \"Get-Process -Name '" + cleanProcessName + "' -ErrorAction SilentlyContinue | Select-Object -First 1 | ForEach-Object { $_.Id }\"";

    std::string result = executeCommand(command.c_str());

    if (!result.empty())
    {
        try
        {
            result.erase(result.find_last_not_of(" \n\r\t") + 1);
            if (!result.empty() && std::all_of(result.begin(), result.end(), ::isdigit))
            {
                return static_cast<DWORD>(std::stoul(result));
            }
        }
        catch (const std::exception &) {}
    }

    return 0;
}

bool AppManager::stopApp(const std::string &processName)
{
    if (processName.empty()) return false;

    std::string actualProcessName = processName;
    if (actualProcessName.length() < 4 || actualProcessName.substr(actualProcessName.length() - 4) != ".exe")
    {
        actualProcessName += ".exe";
    }

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
        std::cout << "Failed to stop process. Trying alternative name...\n";
        if (processName.length() >= 4 && processName.substr(processName.length() - 4) == ".exe")
        {
            std::string nameWithoutExt = processName.substr(0, processName.length() - 4);
            std::string backupCommand = "taskkill /IM \"" + nameWithoutExt + "\" /F";
            int backupResult = system(backupCommand.c_str());
            if (backupResult == 0) return true;
        }
        return false;
    }
}