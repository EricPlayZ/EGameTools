#pragma once
#include <basetsd.h>
#include "..\buffer.h"

namespace GamePH {
	class SessionCooperativeDI;

	class GameDI_PH {
	public:
		union {
			buffer<0x130, SessionCooperativeDI*> pSessionCooperativeDI;
			buffer<0x880, bool> blockPauseGameOnPlayerAfk;
		};

		float GetGameTimeDelta();
		DWORD64 GetCurrentGameVersion();
		void TogglePhotoMode(bool doNothing = false, bool setAsOptionalCamera = false);

		static GameDI_PH* Get();
	};
}