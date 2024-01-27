#pragma once
#include <string>
#include "..\core.h"

namespace Menu {
	namespace Player {
		extern KeyBindOption godMode;
		extern KeyBindOption freezePlayer;
		extern Option playerVariables;
		extern Option disableOutOfBoundsTimer;

		extern std::string saveSCRPath;
		extern std::string loadSCRFilePath;

		extern void Update();
		extern void Render();
	}
}