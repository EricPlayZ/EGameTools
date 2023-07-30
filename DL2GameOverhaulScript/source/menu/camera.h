#pragma once
#include "..\core.h"

namespace Menu {
	namespace Camera {
		extern const int BaseFOV;
		extern int FOV;

		extern bool freeCamEnabled;
		extern SMART_BOOL disablePhotoModeLimitsEnabled;

		extern void Render();
	}
}