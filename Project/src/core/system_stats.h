#pragma once
#include <string>

struct SystemStats {
    float cpu_percent;
    float mem_used_gb;
    float mem_total_gb;
    float mem_percent;
    int process_count;
    std::string uptime;
};

class SystemMonitor {
public:
    static SystemStats getStats();
    
private:
    static float getCPUUsage();
    static void getMemoryInfo(float& used_gb, float& total_gb, float& percent);
    static int getProcessCount();
    static std::string getUptime();
};