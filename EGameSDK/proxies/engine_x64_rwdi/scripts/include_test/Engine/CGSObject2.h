#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK {
	namespace GamePH {
		class LogicalPlayer;
	}

	namespace Engine {
		class EGameSDK_API CGSObject2 {
		public:
			union {
				ClassHelpers::StaticBuffer<0x20, GamePH::LogicalPlayer*> pLogicalPlayer;
			};

			static CGSObject2* Get();
		};
	}
}