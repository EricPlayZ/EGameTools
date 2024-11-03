#pragma once
#include "..\buffer.h"

namespace Engine {
	class CoPhysicsProperty;
}

namespace GamePH {
	class PlayerObjProperties {
	public:
		union {
			buffer<0xF0, Engine::CoPhysicsProperty*> pCoPhysicsProperty;
			//buffer<0x2E80, bool> isOutOfBounds;
			//buffer<0x2E84, float> outOfBoundsTimer;
			buffer<0x35E9, bool> enableTPPModel1;
			buffer<0x35EA, bool> enableTPPModel2;
		};

		static PlayerObjProperties* Get();
	};
}