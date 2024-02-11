#pragma once
#include "..\buffer.h"

namespace Engine {
	class CGSObject2;

	class CLevel2 {
	public:
		union {
			buffer<0x28, CGSObject2*> pCGSObject2;
		};

		static CLevel2* Get();
	};
}