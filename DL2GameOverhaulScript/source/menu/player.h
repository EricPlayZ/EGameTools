#pragma once
#include "..\core.h"

namespace Menu {
	namespace Player {
		extern SMART_BOOL godModeEnabled;
		extern SMART_BOOL freezePlayerEnabled;

		extern void Update();
		extern void Render();
	}
}