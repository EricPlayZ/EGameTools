#pragma once
#include "..\buffer.h"

namespace GamePH {
	class LogicalPlayer;
}

namespace Engine {
	class CGSObject2 {
	public:
		union {
			buffer<0x20, GamePH::LogicalPlayer*> pLogicalPlayer;
		};

		static CGSObject2* Get();
	};
}