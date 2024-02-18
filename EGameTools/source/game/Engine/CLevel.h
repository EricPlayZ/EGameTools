#pragma once
#include "..\buffer.h"

namespace GamePH {
	class LevelDI;
}

namespace Engine {
	class CGSObject;

	class CLevel {
	public:
		union {
			buffer<0x20, GamePH::LevelDI*> pLevelDI;
			buffer<0x30, CGSObject*> pCGSObject;
		};

		static CLevel* Get();
	};
}