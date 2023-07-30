#include <iostream>
#include "print.h"
#include "core.h"
#include "memory.h"

const int refreshConsoleIntervalMs = 250;

void RefreshConsole(const std::string& text = "", const int& x = 0, const int& y = 0) {
	static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	std::cout.flush();

	COORD coord = { (SHORT)x, (SHORT)y };
	SetConsoleCursorPosition(hOut, coord);
	if (!text.empty())
		std::cout << text;
}
COORD GetCurrentCursorPos() {
	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(hConsole, &csbi))
		return csbi.dwCursorPosition;

	return COORD{ 0, 0 };
}
int GetCurrentCursorPosX() {
	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(hConsole, &csbi))
		return csbi.dwCursorPosition.X;

	return -1;
}
int GetCurrentCursorPosY() {
	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(hConsole, &csbi))
		return csbi.dwCursorPosition.Y;

	return -1;
}

void PrintSpaces(const int& spaceCount) {
	std::string spaces = " ";
	for (int i = 0; i < spaceCount - 2; ++i) {
		spaces += " ";
	}

	printf(spaces.c_str());
}

void DrawConsoleOut() {
	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
		int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		int consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

		RefreshConsole();
	}
}