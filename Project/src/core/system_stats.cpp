#include "system_stats.h"
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <tlhelp32.h>
#include <sstream>
#include <iomanip>
#define UNICODE
#define _UNICODE


#pragma comment(lib, "pdh.lib")

SystemStats SystemMonitor::getStats() {
    SystemStats stats;
    
    stats.cpu_percent = getCPUUsage();
    getMemoryInfo(stats.mem_used_gb, stats.mem_total_gb, stats.mem_percent);
    stats.process_count = getProcessCount();
    stats.uptime = getUptime();
    
    return stats;
}

float SystemMonitor::getCPUUsage() {
    static PDH_HQUERY query = NULL;
    static PDH_HCOUNTER counter = NULL;
    static bool initialized = false;
    
    if (!initialized) {
        PdhOpenQuery(NULL, 0, &query);
        // SỬA: Dùng PdhAddCounterW cho Unicode hoặc PdhAddCounterA cho ANSI
        PdhAddCounterW(query, L"\\Processor(_Total)\\% Processor Time", 0, &counter);
        PdhCollectQueryData(query);
        initialized = true;
        Sleep(100); // Wait for first sample
    }
    
    PDH_FMT_COUNTERVALUE value;
    PdhCollectQueryData(query);
    PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &value);
    
    return static_cast<float>(value.doubleValue);
}

void SystemMonitor::getMemoryInfo(float& used_gb, float& total_gb, float& percent) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
    DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    
    total_gb = totalPhysMem / (1024.0f * 1024.0f * 1024.0f);
    used_gb = physMemUsed / (1024.0f * 1024.0f * 1024.0f);
    percent = memInfo.dwMemoryLoad;
}

int SystemMonitor::getProcessCount() {
    int count = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        
        if (Process32First(hSnapshot, &pe32)) {
            do {
                count++;
            } while (Process32Next(hSnapshot, &pe32));
        }
        
        CloseHandle(hSnapshot);
    }
    
    return count;
}

std::string SystemMonitor::getUptime() {
    DWORD ticks = GetTickCount64() / 1000; // Convert to seconds
    
    int hours = ticks / 3600;
    int minutes = (ticks % 3600) / 60;
    
    std::ostringstream oss;
    oss << hours << "h " << minutes << "m";
    return oss.str();
}