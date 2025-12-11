#include "menu_display.h"
#include <iostream>
#include <iomanip>
#include <windows.h>

using namespace std;

// Color constants
const int COLOR_HEADER_BG = 11;
const int COLOR_HEADER_FG = 0;
const int COLOR_SECTION = 14;
const int COLOR_COMMAND = 15;
const int COLOR_PARAM = 8;
const int COLOR_DESC = 7;
const int COLOR_BORDER = 8;
const int COLOR_DEFAULT = 7;

void setColor(int foreground, int background)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), foreground | (background * 16));
}

int getConsoleWidth()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

void printCentered(const string &text, int width, int fg_color, int bg_color)
{
	setColor(COLOR_DEFAULT);
	int padding = (width - text.length()) / 2;
	cout << string(width, ' ') << "\n";
	setColor(fg_color, bg_color);
	cout << string(padding, ' ');
	cout << text;
	cout << string(width - padding - text.length(), ' ') << endl;
	setColor(COLOR_DEFAULT);
}

void printCommand(const string &command, const string &description, int width, const string &param)
{
	int commandColWidth = width * 0.45;

	cout << "  ";
	setColor(COLOR_COMMAND);
	cout << left << setw(20) << command;

	setColor(COLOR_PARAM);
	cout << left << setw(commandColWidth - 22) << param;

	setColor(COLOR_DESC);
	cout << description << endl;
}

void printLine(char c, int width, int color)
{
	setColor(color);
	cout << string(width, c) << endl;
}

void displayMenu()
{
	system("cls");

	int consoleWidth = getConsoleWidth();

	printCentered("REMOTE CONTROL CLIENT - DUAL SOCKET ARCHITECTURE", consoleWidth, COLOR_HEADER_FG, COLOR_HEADER_BG);

	setColor(COLOR_PARAM);
	cout << " * All logs saved to: " << logFileName << endl;
	setColor(COLOR_DEFAULT);

	cout << endl;
	setColor(COLOR_SECTION);
	cout << " SYSTEM CONTROL" << endl;
	printLine('-', consoleWidth, COLOR_BORDER);
	printCommand("PROCESS_LIST", "List running processes", consoleWidth);
	printCommand("APP_LIST", "List installed applications", consoleWidth);
	printCommand("APP_START", "Start application", consoleWidth, "<path>");
	printCommand("APP_STOP", "Stop application", consoleWidth, "<name/path>");
	printCommand("START", "Start system process", consoleWidth, "<path>");
	printCommand("STOP", "Stop system process", consoleWidth, "<process>");
	printCommand("SHUTDOWN", "Shutdown server", consoleWidth);
	printCommand("RESTART", "Restart server", consoleWidth);

	cout << endl;
	setColor(COLOR_SECTION);
	cout << " MONITORING & CAPTURE" << endl;
	printLine('-', consoleWidth, COLOR_BORDER);
	printCommand("KEYLOG", "Start keylogger", consoleWidth);
	printCommand("STOPKEYLOG", "Stop keylogger", consoleWidth);
	printCommand("SCREEN_CAPTURE", "Capture server screen", consoleWidth);
	printCommand("LIVESTREAM", "Start video streaming", consoleWidth);
	printCommand("STOPLIVESTREAM", "Stop livestream & get video", consoleWidth);

	cout << endl;
	setColor(COLOR_SECTION);
	cout << " FILE OPERATIONS" << endl;
	printLine('-', consoleWidth, COLOR_BORDER);
	printCommand("GET", "Download file from server", consoleWidth, "<file>");
	printCommand("LS", "List directory contents", consoleWidth, "<path>");

	cout << endl;
	printLine('=', consoleWidth, COLOR_BORDER);
	printCommand("EXIT", "Disconnect from server", consoleWidth);
	printLine('=', consoleWidth, COLOR_BORDER);

	setColor(COLOR_DEFAULT);
	cout << "\n> Enter command: ";
}
