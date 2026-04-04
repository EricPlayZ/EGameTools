#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK {
	namespace GamePH {
		class LevelDI;
	}

	namespace Engine {
		class EGameSDK_API CLevel {
		public:
			ClassHelpers::StaticBuffer<0x20, GamePH::LevelDI*> pLevelDI;

			static CLevel* Get();
		};
	}
}
