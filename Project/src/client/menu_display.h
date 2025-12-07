#pragma once

#include <string>

// Forward declarations for external variables
extern std::string logFileName;

// Menu display functionality
void displayMenu();

// Helper functions for console formatting
void setColor(int foreground, int background = 0);
int getConsoleWidth();
void printCentered(const std::string &text, int width, int fg_color, int bg_color);
void printCommand(const std::string &command, const std::string &description, int width, const std::string &param = "");
void printLine(char c, int width, int color);