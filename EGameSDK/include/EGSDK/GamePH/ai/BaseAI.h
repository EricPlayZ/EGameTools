#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	namespace ai {
		class EGameSDK_API BaseAI {
		public:
			union {
				DynamicField(BaseAI, void*, pBaseAIManager);
			};
		};
	}
}
