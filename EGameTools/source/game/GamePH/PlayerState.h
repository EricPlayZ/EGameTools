#pragma once
#include "..\buffer.h"

namespace GamePH {
	class PlayerVariables;

	class PlayerState {
	public:
		union {
			buffer<0x2B0, PlayerVariables*> playerVars;
		};

		static PlayerState* Get();
	};
}