#pragma once
#include "..\buffer.h"

namespace GamePH {
	class PlayerObjProperties;

	class LocalClientDI {
	public:
		union {
			buffer<0x90, PlayerObjProperties*> pPlayerObjProperties;
		};

		static LocalClientDI* Get();
	};
}