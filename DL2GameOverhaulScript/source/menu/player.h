#pragma once
#include <string>
#include "..\core.h"

namespace Menu {
	namespace Player {
		extern SMART_BOOL godModeEnabled;
		extern SMART_BOOL freezePlayerEnabled;

		extern bool useBACKUPPlayerVarsEnabled;

		extern std::string saveSCRPath;
		extern std::string loadSCRFilePath;

		extern bool restoreVarsToSavedVarsEnabled;

		extern void Update();
		extern void Render();
	}
}