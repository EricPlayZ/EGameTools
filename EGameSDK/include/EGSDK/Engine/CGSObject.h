#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::Engine {
	class CLevel2;

	class EGameSDK_API CGSObject {
	public:
		union {
			ClassHelpers::StaticBuffer<0x48, CLevel2*> pCLevel2;
		};

		static CGSObject* Get();
	};
}