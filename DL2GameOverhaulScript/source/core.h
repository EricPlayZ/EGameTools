#pragma once
#include <Windows.h>

struct SMART_BOOL {
	bool previousValue;
	bool value;

	SMART_BOOL() {
		changed = false;

		previousValue = false;
		value = false;
	}

	bool Change(bool newValue) {
		if (changed && newValue == value)
			return false;

		if (!changed)
			previousValue = value;

		changed = true;
		value = newValue;
		return true;
	}
	bool Restore() {
		if (!changed)
			return false;

		changed = false;

		value = previousValue;
		return true;
	}

private:
	bool changed;
};

namespace Core {
	extern void DisableConsole();

	extern bool exiting;
	extern DWORD64 WINAPI MainThread(HMODULE hModule);

	extern void Cleanup();
}