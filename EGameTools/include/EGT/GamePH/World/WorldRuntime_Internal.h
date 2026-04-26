#pragma once

namespace EGT::GamePH::World {
	extern void OnWorldTimeSliderChanged(float timeValue);
	extern void SetWorldIsModifyingGameSpeed(bool value);
	extern void UpdateWorldTimeRuntime();
	extern void RequestWorldWeatherInterpolation();
	extern void UpdateWorldWeatherRuntime();
	extern void UpdateWorldSnowRuntime();
}
