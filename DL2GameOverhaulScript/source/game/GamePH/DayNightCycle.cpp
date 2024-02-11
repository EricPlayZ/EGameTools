#include <pch.h>
#include "..\offsets.h"
#include "DayNightCycle.h"

namespace GamePH {
	void DayNightCycle::SetDaytime(float time) {
		time /= 24;
		time1 = time;
		time2 = time;
		time3 = time;
	}

	DayNightCycle* DayNightCycle::Get() {
		__try {
			if (!Offsets::Get_g_DayNightCycle())
				return nullptr;

			DayNightCycle* ptr = *reinterpret_cast<DayNightCycle**>(Offsets::Get_g_DayNightCycle());
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}