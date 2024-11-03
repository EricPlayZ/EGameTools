#pragma once
#include <basetsd.h>
#include "..\buffer.h"

namespace GamePH {
	class SessionCooperativeDI;

	class GameDI_PH {
	public:
		union {
			buffer<0x130, SessionCooperativeDI*> pSessionCooperativeDI;
			buffer<0x908, bool> blockPauseGameOnPlayerAfk;
		};

		float GetGameTimeDelta();
		void TogglePhotoMode(bool doNothing = false, bool setAsOptionalCamera = false);

		static GameDI_PH* Get();
	};
}