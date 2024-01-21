#pragma once
#include <string>
#include <Hotkey.h>
#include "..\core.h"

namespace Menu {
	namespace Player {
		extern SMART_BOOL godModeEnabled;
		extern KeyBindToggle godModeToggleKey;
		extern SMART_BOOL freezePlayerEnabled;
		extern KeyBindToggle freezePlayerToggleKey;
		extern bool playerVariablesEnabled;

		extern std::string saveSCRPath;
		extern std::string loadSCRFilePath;

		extern void Update();
		extern void Render();
	}
}