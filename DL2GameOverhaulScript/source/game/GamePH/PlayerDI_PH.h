#pragma once
#include "..\buffer.h"

namespace GamePH {
	class PlayerDI_PH {
	public:
		bool IsInAir();

		static PlayerDI_PH* Get();
	};
}