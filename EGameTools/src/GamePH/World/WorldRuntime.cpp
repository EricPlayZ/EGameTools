#include <EGT\GamePH\World\WorldRuntime_Internal.h>
#include <EGT\GamePH\World\WorldRuntime.h>

namespace EGT::GamePH::World {
	void OnTimeSliderChanged(float timeValue) {
		OnWorldTimeSliderChanged(timeValue);
		RequestWorldWeatherInterpolation();
	}

	void SetIsModifyingGameSpeed(bool value) {
		SetWorldIsModifyingGameSpeed(value);
	}

	void RequestWeatherInterpolation() {
		RequestWorldWeatherInterpolation();
	}

	void UpdateRuntimeState() {
		UpdateWorldTimeRuntime();
		UpdateWorldWeatherRuntime();
		UpdateWorldSnowRuntime();
	}
}
