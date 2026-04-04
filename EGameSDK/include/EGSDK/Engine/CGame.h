#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK {
	namespace GamePH {
		class GameDI_PH;
	}

	namespace Engine {
		class CVideoSettings;
		class CLevel;

		class EGameSDK_API CGame {
		public:
			union {
				DynamicField(CGame, GamePH::GameDI_PH*, pGameDI_PH);
				DynamicField(CGame, CVideoSettings*, pCVideoSettings);
				DynamicField(CGame, CLevel*, pCLevel);
			};

			static CGame* Get();
		};
	}
}