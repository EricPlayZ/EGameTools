#pragma once

namespace EGT::GamePH::World {
	extern void OnTimeSliderChanged(float timeValue);
	extern void SetIsModifyingGameSpeed(bool value);
	extern void RequestWeatherInterpolation();
	extern void UpdateRuntimeState();
}
