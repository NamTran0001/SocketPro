#include "power_control.h"
#include <cstdlib>   

void shutdownComputer() {
    system("shutdown /s /t 0");
}

void restartComputer() {
    system("shutdown /r /t 0");
}

void stopComputer() {
    system("shutdown /s /t 0");
}
