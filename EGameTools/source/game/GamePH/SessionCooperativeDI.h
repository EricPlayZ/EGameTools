#pragma once
#include "..\buffer.h"

namespace GamePH {
	class LocalClientDI;

	class SessionCooperativeDI {
	public:
		union {
			buffer<0xE08, LocalClientDI*> pLocalClientDI;
		};

		static SessionCooperativeDI* Get();
	};
}