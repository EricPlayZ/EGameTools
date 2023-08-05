#pragma once
#include <Windows.h>
#include <string>
#include "config\time_tools.h"

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

template<typename... Args> std::string PrintError(std::string f, Args... args) {
	f.append("\n");
	std::ostringstream oss = GetTimestamp();
	oss << f;
	const std::string ossStr = oss.str();

	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c_red);
	printf(ossStr.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white);

	return oss.str();
}
template<typename... Args> std::string PrintWarning(std::string f, Args... args) {
	f.append("\n");
	std::ostringstream oss = GetTimestamp();
	oss << f;
	const std::string ossStr = oss.str();

	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c_yellow);
	printf(ossStr.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white);

	return oss.str();
}
template<typename... Args> std::string PrintSuccess(std::string f, Args... args) {
	f.append("\n");
	std::ostringstream oss = GetTimestamp();
	oss << f;
	const std::string ossStr = oss.str();

	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c_green);
	printf(ossStr.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white);

	return oss.str();
}
template<typename... Args> std::string PrintInfo(std::string f, Args... args) {
	f.append("\n");
	std::ostringstream oss = GetTimestamp();
	oss << f;
	const std::string ossStr = oss.str();

	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c_brightwhite);
	printf(ossStr.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white);

	return oss.str();
}
template<typename... Args> std::string PrintWaiting(std::string f, Args... args) {
	f.append("\n");
	std::ostringstream oss = GetTimestamp();
	oss << f;
	const std::string ossStr = oss.str();

	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c_gray);
	printf(ossStr.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white);

	return oss.str();
}
template<typename... Args> std::string PrintCustom(std::string f, const ConsoleColors color, Args... args) {
	f.append("\n");
	std::ostringstream oss = GetTimestamp();
	oss << f;
	const std::string ossStr = oss.str();

	const HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
	printf(ossStr.c_str(), args...);
	SetConsoleTextAttribute(hConsole, c_white);

	return oss.str();
}