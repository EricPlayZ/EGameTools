#pragma once
#include "..\buffer.h"

namespace Engine {
	class CLevel2;

	class CGSObject {
	public:
		union {
			buffer<0x48, CLevel2*> pCLevel2;
		};

		static CGSObject* Get();
	};
}