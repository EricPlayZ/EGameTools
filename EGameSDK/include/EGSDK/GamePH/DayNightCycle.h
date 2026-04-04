#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	class EGameSDK_API DayNightCycle {
	public:
		union {
			DynamicField(DayNightCycle, float, time1);
			DynamicField(DayNightCycle, float, time2);
			DynamicField(DayNightCycle, float, time3);
		};

		void SetDaytime(float time);

		static DayNightCycle* Get();
	};
}