#pragma once
#include <Windows.h>
#include <string>

enum ConsoleColors {
	c_black,
	c_blue,
	c_green,
	c_aqua,
	c_red,
	c_purple,
	c_yellow,
	c_white,
	c_gray,
	c_lightblue,
	c_lightgreen,
	c_lightaqua,
	c_lightred,
	c_lightpurple,
	c_lightyellow,
	c_brightwhite
};

template<typename... Args> void PrintError(std::string f, Args... args) {
	f.insert(0, "[!] ");
	f.append("\n");

	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c_red);
	printf(f.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white);
}
template<typename... Args> void PrintWaiting(std::string f, Args... args) {
	f.insert(0, "[...] ");
	f.append("\n");

	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c_gray);
	printf(f.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white);
}
template<typename... Args> void PrintInfo(std::string f, Args... args) {
	f.insert(0, "[.] ");
	f.append("\n");

	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c_brightwhite);
	printf(f.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white);
}
template<typename... Args> void PrintSuccess(std::string f, Args... args) {
	f.insert(0, "[!] ");
	f.append("\n");

	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c_green);
	printf(f.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white);
}
template<typename... Args> void PrintCustom(const std::string& f, const ConsoleColors& color, Args... args) {
	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
	printf(f.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white); // White
}

extern const int refreshConsoleIntervalMs;

extern void DrawConsoleOut();