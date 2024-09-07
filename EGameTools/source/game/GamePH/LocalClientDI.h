#pragma once
#include "..\buffer.h"

namespace GamePH {
	class PlayerDI_PH;

	class LocalClientDI {
	public:
		union {
			buffer<0x90, PlayerDI_PH*> pPlayerDI_PH;
		};

		static LocalClientDI* Get();
	};
}