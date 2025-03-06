#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::Engine {
	class CGSObject2;

	class EGameSDK_API CLevel2 {
	public:
		union {
			ClassHelpers::StaticBuffer<0x28, CGSObject2*> pCGSObject2;
		};

		static CLevel2* Get();
	};
}