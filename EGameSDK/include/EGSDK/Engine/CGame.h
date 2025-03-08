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
				ClassHelpers::StaticBuffer<0x8, GamePH::GameDI_PH*> pGameDI_PH;
				ClassHelpers::StaticBuffer<0x28, CVideoSettings*> pCVideoSettings;
				DynamicField(CGame, CLevel*, pCLevel);
			};

			static CGame* Get();
		};
	}
}