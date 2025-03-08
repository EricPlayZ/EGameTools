#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::Engine {
	class EGameSDK_API CVideoSettings {
	public:
		union {
			DynamicField(CVideoSettings, float, extraFOV);
		};

		static CVideoSettings* Get();
	};
}