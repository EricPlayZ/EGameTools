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

	void Change(bool newValue) {
		if (changed && newValue == value)
			return;

		changed = true;

		previousValue = value;
		value = newValue;
	}
	void Restore() {
		if (!changed)
			return;

		changed = false;

		value = previousValue;
	}

private:
	bool changed;
};

namespace Core {
	extern void DisableConsole();
	extern DWORD64 WINAPI MainThread(HMODULE hModule);
}